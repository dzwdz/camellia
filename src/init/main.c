#include <init/types.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdint.h>

#define argify(str) str, sizeof(str) - 1
#define log(str) _syscall_write(tty_fd, argify(str))

extern char _bss_start; // provided by the linker
extern char _bss_end;

int tty_fd;

void fs_test(void);
void fs_server(handle_t back);

__attribute__((section(".text.startup")))
int main(void) {
	// allocate bss
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	tty_fd = _syscall_open("/tty", sizeof("/tty") - 1);
	if (tty_fd < 0)
		_syscall_exit(argify("couldn't open tty"));

	fs_test();
	_syscall_exit(argify("my job here is done."));
}

void fs_test(void) {
	handle_t front, back, file;
	front = _syscall_fs_create(&back);

	if (_syscall_fork()) {
		// child: is the fs server
		fs_server(back);
		return;
	}

	// parent: accesses the fs
	_syscall_mount(front, argify("/mnt"));
	log("requesting file. ");
	file = _syscall_open(argify("/mnt/tty"));
	log("open returned. ");
	_syscall_write(file, argify("hello"));
}

void fs_server(handle_t back) {
	static char buf[64];
	int len, id;
	for (;;) {
		len = 64;
		switch (_syscall_fs_wait(back, buf, &len, &id)) {
			case VFSOP_OPEN:
				_syscall_write(tty_fd, buf, len);
				log(" was opened. ");
				_syscall_fs_respond(NULL, 32); // doesn't check the path yet
				break;

			case VFSOP_WRITE:
				// uppercase the buffer
				for (int i = 0; i < len; i++) buf[i] &= ~id; // id == 32
				// and passthrough to tty
				_syscall_write(tty_fd, buf, len);
				_syscall_fs_respond(NULL, len); // return the amt of bytes written
				break;

			default:
				log("fuck");
				break;
		}
	}

}

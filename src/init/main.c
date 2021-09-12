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

int main(void) {
	// allocate bss
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	tty_fd = _syscall_open("/tty", sizeof("/tty") - 1);
	if (tty_fd < 0)
		_syscall_exit(argify("couldn't open tty"));
	log(" opened /tty ");

	fs_test();
	_syscall_exit(argify("my job here is done."));
}

void fs_test(void) {
	static char buf[64];
	int len = 64;
	handle_t front, back, file;
	front = _syscall_fs_create(&back);

	if (_syscall_fork()) {
		// child: is the fs server
		log("fs_wait started. ");
		_syscall_fs_wait(back, buf, &len);
		log("fs_wait returned. ");
		_syscall_write(tty_fd, buf, len);
	} else {
		// parent: accesses the fs
		_syscall_mount(front, argify("/mnt"));
		log("requesting file. ");
		file = _syscall_open(argify("/mnt/test"));
	}
}

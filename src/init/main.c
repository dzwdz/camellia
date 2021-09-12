#include <init/types.h>
#include <shared/syscalls.h>
#include <stdint.h>

#define argify(str) str, sizeof(str) - 1
#define log(str) _syscall_write(tty_fd, argify(str))

__attribute__((section("text")))
int tty_fd;

void misc_tests(void);
void fs_test(void);
// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str);

#define mount(fd, path) _syscall_mount(fd, path, sizeof(path)-1)

int main(void) {
	tty_fd = _syscall_open("/tty", sizeof("/tty") - 1);
	if (tty_fd < 0)
		_syscall_exit(argify("couldn't open tty"));
	log(" opened /tty ");

	misc_tests();
	fs_test();

	_syscall_exit(argify("my job here is done."));
}

void fs_test(void) {
	static char buf[64] __attribute__((section("text")));
	int len = 64;
	handle_t front, back, file;
	front = _syscall_fs_create((void*)&back); // TODO change user_ptr to void*

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

void misc_tests(void) {
	int res;
	res = _syscall_open(argify("/tty/nonexistant"));
	if (res >= 0) log("test failed ");
	res = _syscall_open(argify("/ttynonexistant"));
	if (res >= 0) log("test failed ");
	log("the \"tests\" went ok");
}

// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str) {
	static char buf[0x2000] __attribute__((section("text")));

	char *out = (void*)((uintptr_t)buf & ~0xFFF) + 0xFFF;
	char *cur = out;
	do {
		*cur++ = *str;
	} while (*str++ != '\0');
	return out;
}

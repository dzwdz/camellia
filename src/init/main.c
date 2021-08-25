#include <init/types.h>
#include <shared/syscalls.h>
#include <stdint.h>

#define argify(str) str, sizeof(str) - 1
#define log(str) _syscall_fd_write(tty_fd, argify(str))

__attribute__((section("text")))
int tty_fd;

void misc_tests(void);
// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str);

#define mount(fd, path) _syscall_fd_mount(fd, path, sizeof(path)-1)

int main(void) {
	// TODO don't dispatch /ttywhatever to the tty driver

	tty_fd = _syscall_fs_open("/tty", sizeof("/tty") - 1);
	if (tty_fd < 0)
		_syscall_exit(argify("couldn't open tty"));
	log(" opened /tty ");

	misc_tests();

	_syscall_exit(argify("my job here is done."));
}

void misc_tests(void) {
	int res;
	res = _syscall_fs_open(argify("/tty/nonexistant"));
	if (res >= 0) log("test failed ");
	res = _syscall_fs_open(argify("/ttynonexistant"));
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

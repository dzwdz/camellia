#include <init/types.h>
#include <shared/magic.h>
#include <shared/syscalls.h>
#include <stdint.h>

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

#define mount(fd, path) _syscall_fd_mount(fd, path, sizeof(path)-1)

int main(void) {
	int fd;

	mount(0, "/some/where");
	mount(0, "/some");

	_syscall_fs_open(  // should fail
			multipageify("/some/where/file"),
			      sizeof("/some/where/file") - 1);

	fd = _syscall_fs_open(
			multipageify("/tty"),
			      sizeof("/tty") - 1);
	// TODO don't dispatch /ttywhatever to the tty driver

	if (fd < 0) {
		_syscall_exit("couldn't open tty",
		       sizeof("couldn't open tty") - 1);
	}

	_syscall_fd_write(fd, "fd test ", 8);
	_syscall_fd_close(fd);
	_syscall_fd_write(fd, "fd test ", 8); // should fail

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}

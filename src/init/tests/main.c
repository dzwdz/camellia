#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/syscalls.h>

#define argify(str) str, sizeof(str) - 1

static void read_file(const char *path, size_t len) {
	int fd = _syscall_open(path, len);
	static char buf[64];
	int buf_len = 64;

	_syscall_write(__tty_fd, path, len, 0);
	printf(": ");
	if (fd < 0) {
		printf("couldn't open.\n");
		return;
	}

	buf_len = _syscall_read(fd, buf, buf_len, 0);
	_syscall_write(__tty_fd, buf, buf_len, 0);

	_syscall_close(fd);
}

void test_all(void) {
	test_fs();
	test_await();
}

void test_fs(void) {
	if (!_syscall_fork()) {
		/* run the "test" in a child process to not affect the fs view of the
		 * main process */
		read_file(argify("/init/fake.txt"));
		read_file(argify("/init/1.txt"));
		read_file(argify("/init/2.txt"));
		read_file(argify("/init/dir/3.txt"));

		printf("\nshadowing /init/dir...\n");
		_syscall_mount(-1, argify("/init/dir"));
		read_file(argify("/init/fake.txt"));
		read_file(argify("/init/1.txt"));
		read_file(argify("/init/2.txt"));
		read_file(argify("/init/dir/3.txt"));

		printf("\n");
		_syscall_exit(0);
	} else _syscall_await();
}

void test_await(void) {
	int ret;

	if (!_syscall_fork()) {
		/* this "test" runs in a child process, because otherwise it would be
		 * stuck waiting for e.g. the tar_driver process to exit */

		// regular exit()s
		if (!_syscall_fork()) _syscall_exit(69);
		if (!_syscall_fork()) _syscall_exit(420);

		// faults
		if (!_syscall_fork()) { // invalid memory access
			asm volatile("movb $69, 0" ::: "memory");
			printf("this shouldn't happen");
			_syscall_exit(-1);
		}
		if (!_syscall_fork()) { // #GP
			asm volatile("hlt" ::: "memory");
			printf("this shouldn't happen");
			_syscall_exit(-1);
		}

		while ((ret = _syscall_await()) != ~0)
			printf("await returned: %x\n", ret);
		printf("await: no more children\n");

		_syscall_exit(0);
	} else _syscall_await();
}

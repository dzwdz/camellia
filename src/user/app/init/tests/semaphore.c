#define TEST_MACROS
#include "tests.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/esemaphore.h>

static void odd(handle_t out, struct evil_sem *sem1, struct evil_sem *sem2) {
	_syscall_write(out, "1", 1, -1);
	esem_signal(sem1);

	esem_wait(sem2);
	_syscall_write(out, "3", 1, -1);
	esem_signal(sem1);

	esem_wait(sem2);
	_syscall_write(out, "5", 1, -1);
	esem_signal(sem1);
}

static void even(handle_t out, struct evil_sem *sem1, struct evil_sem *sem2) {
	esem_wait(sem1);
	_syscall_write(out, "2", 1, -1);
	esem_signal(sem2);

	esem_wait(sem1);
	_syscall_write(out, "4", 1, -1);
	esem_signal(sem2);

	esem_wait(sem1);
	_syscall_write(out, "6", 1, -1);
	esem_signal(sem2);
}

void test_semaphore(void) {
	struct evil_sem *sem1, *sem2;
	handle_t pipe[2];
	assert(_syscall_pipe(pipe, 0) >= 0);

	if (!fork()) {
		sem1 = esem_new(0);
		sem2 = esem_new(0);
		assert(sem1 && sem2);
		if (!fork()) {
			odd(pipe[1], sem1, sem2);
			exit(69);
		} else {
			even(pipe[1], sem1, sem2);
			assert(_syscall_await() == 69);
		}
		esem_free(sem1);
		esem_free(sem2);

		_syscall_write(pipe[1], "|", 1, -1);

		sem1 = esem_new(0);
		sem2 = esem_new(0);
		assert(sem1 && sem2);
		if (!fork()) {
			even(pipe[1], sem1, sem2);
			exit(69);
		} else {
			odd(pipe[1], sem1, sem2);
			assert(_syscall_await() == 69);
			_syscall_await();
		}
		esem_free(sem1);
		esem_free(sem2);

		exit(0);
	} else {
		close(pipe[1]);

		char buf[16];
		size_t pos = 0;
		for (;;) {
			int ret = _syscall_read(pipe[0], buf + pos, sizeof(buf) - pos, 0);
			if (ret < 0) break;
			pos += ret;
		}
		buf[pos] = '\0'; // idc about the "potential" overflow
		if (strcmp(buf, "123456|123456")) {
			printf("%s\n", buf);
			test_fail();
		}

		_syscall_await();
	}
}

#include "../tests.h"
#include <camellia/flags.h>
#include <errno.h>
#include <unistd.h>

static void test_fdlimit_pipe(void) {
	handle_t ends[2];
	handle_t h[2] = {-1, -1};
	for (;;) {
		handle_t cur = _syscall_open("/", 1, OPEN_READ);
		if (cur == -EMFILE) break;
		test(cur >= 0);
		h[0] = h[1];
		h[1] = cur;
	}
	test(h[0] >= 0); /* we need at least two handles */

	test(_syscall_pipe(ends, 0) == -EMFILE);
	test(_syscall_open("/", 1, OPEN_READ) == -EMFILE);

	close(h[1]);
	test(_syscall_pipe(ends, 0) == -EMFILE);
	test(_syscall_open("/", 1, OPEN_READ) == h[1]);
	test(_syscall_open("/", 1, OPEN_READ) == -EMFILE);

	close(h[0]);
	close(h[1]);
	test(_syscall_pipe(ends, 0) == 0);
}

static void test_fdlimit_fork(void) {
	for (;;) {
		handle_t cur = _syscall_open("/", 1, OPEN_READ);
		if (cur == -EMFILE) break;
		test(cur >= 0);
	}

	if (!_syscall_fork(0, NULL)) _syscall_exit(123);

	test(_syscall_fork(FORK_NEWFS, NULL) == -EMFILE);
	test(_syscall_await() == 123);
	test(_syscall_await() == ~0);
}

void r_k_fdlimit(void) {
	/* all these tests implicitly test if the vfs returns -EMFILE */
	run_test(test_fdlimit_pipe);
	run_test(test_fdlimit_fork);
}

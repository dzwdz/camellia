#include "../tests.h"
#include <camellia/flags.h>
#include <errno.h>
#include <unistd.h>

static void test_fdlimit_pipe(void) {
	hid_t ends[2];
	hid_t h[2] = {-1, -1};
	for (;;) {
		hid_t cur = _sys_open("/", 1, OPEN_READ);
		if (cur == -EMFILE) break;
		test(cur >= 0);
		h[0] = h[1];
		h[1] = cur;
	}
	test(h[0] >= 0); /* we need at least two handles */

	test(_sys_pipe(ends, 0) == -EMFILE);
	test(_sys_open("/", 1, OPEN_READ) == -EMFILE);

	close(h[1]);
	test(_sys_pipe(ends, 0) == -EMFILE);
	test(_sys_open("/", 1, OPEN_READ) == h[1]);
	test(_sys_open("/", 1, OPEN_READ) == -EMFILE);

	close(h[0]);
	close(h[1]);
	test(_sys_pipe(ends, 0) == 0);
}

static void test_fdlimit_fork(void) {
	for (;;) {
		hid_t cur = _sys_open("/", 1, OPEN_READ);
		if (cur == -EMFILE) break;
		test(cur >= 0);
	}

	if (!_sys_fork(0, NULL)) _sys_exit(123);

	test(_sys_fork(FORK_NEWFS, NULL) == -EMFILE);
	test(_sys_await() == 123);
	test(_sys_await() == ~0);
}

void r_k_fdlimit(void) {
	/* all these tests implicitly test if the vfs returns -EMFILE */
	run_test(test_fdlimit_pipe);
	run_test(test_fdlimit_fork);
}

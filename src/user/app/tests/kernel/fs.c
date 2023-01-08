// TODO test it working too...

#include "../tests.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>

static void test_unfinished_req(void) {
	handle_t h = -1;
	int ret = _syscall_fork(FORK_NEWFS, &h);
	test(0 <= ret);
	if (ret == 0) {
		// TODO make a similar test with all 0s passed to fs_wait
		struct ufs_request res;
		_syscall_fs_wait(NULL, 0, &res);
		// TODO second fs_wait
		exit(0);
	} else {
		test(0 <= h);
		test(_syscall_mount(h, "/", 1) == 0);
		int ret = _syscall_open("/", 1, 0);
		test(ret < 0);
		// the handler quits while handling that call - but this syscall should return anyways
	}
}

static void test_orphaned_fs(void) {
	handle_t h = -1;
	int ret = _syscall_fork(FORK_NEWFS, &h);
	test(0 <= ret);
	if (ret == 0) {
		exit(0);
	} else {
		test(0 <= h);
		test(_syscall_mount(h, "/", 1) == 0);
		int ret = _syscall_open("/", 1, 0);
		test(ret < 0);
	}
}

void r_k_fs(void) {
	run_test(test_unfinished_req);
	run_test(test_orphaned_fs);
}

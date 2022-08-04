#include "../tests.h"
#include <camellia/syscalls.h>

static void test_unfinished_req(void) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h)) {
		// TODO make a similar test with all 0s passed to fs_wait
		struct fs_wait_response res;
		_syscall_fs_wait(NULL, 0, &res);
		// TODO second fs_wait
		exit(0);
	} else {
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		test(ret < 0);
		// the handler quits while handling that call - but this syscall should return anyways
	}
}

static void test_orphaned_fs(void) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h)) {
		exit(0);
	} else {
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		test(ret < 0);
	}
}

void r_k_fs(void) {
	run_test(test_unfinished_req);
	run_test(test_orphaned_fs);
}

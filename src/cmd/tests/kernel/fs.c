#include "../tests.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <string.h>

static void test_unfinished_req(void) {
	hid_t h = -1;
	int ret = _sys_fork(FORK_NEWFS, &h);
	test(0 <= ret);
	if (ret == 0) {
		// TODO make a similar test with all 0s passed to fs_wait
		struct ufs_request res;
		_sys_fs_wait(NULL, 0, &res);
		// TODO second fs_wait
		exit(0);
	} else {
		test(0 <= h);
		test(_sys_mount(h, "/", 1) == 0);
		int ret = _sys_open("/", 1, 0);
		test(ret < 0);
		// the handler quits while handling that call - but this syscall should return anyways
	}
}

static void test_orphaned_fs(void) {
	hid_t h = -1;
	int ret = _sys_fork(FORK_NEWFS, &h);
	test(0 <= ret);
	if (ret == 0) {
		exit(0);
	} else {
		test(0 <= h);
		test(_sys_mount(h, "/", 1) == 0);
		int ret = _sys_open("/", 1, 0);
		test(ret < 0);
	}
}

static void test_fs_cleanup(void) {
	const char *msg = "success";
	int msglen = 8;
	char buf[16];
	hid_t ends[2];
	test(_sys_pipe(ends, 0) >= 0);
	if (!_sys_fork(0, NULL)) {
		hid_t h = -1;
		if (_sys_fork(FORK_NEWFS, &h) == 0) {
			for (;;) {
				struct ufs_request req;
				hid_t reqh = _sys_fs_wait(buf, sizeof buf, &req);
				if (reqh < 0) break;
				_sys_fs_respond(reqh, NULL, 0, 0); /* success */
			}
			/* this is the test: does it break out of the loop
			 * when it should cleanup */
			_sys_write(ends[1], msg, msglen, -1, 0);
			exit(0);
		} else {
			test(_sys_mount(h, "/", 1) == 0);
			h = _sys_open("/", 1, 0);
			test(h >= 0);
			_sys_close(h);
			// TODO another test without the delay
			_sys_sleep(0);
			exit(0);
		}
	} else {
		test(_sys_read(ends[0], buf, sizeof buf, 0) == msglen);
		test(memcmp(buf, msg, msglen) == 0);
	}
}

void r_k_fs(void) {
	run_test(test_unfinished_req);
	run_test(test_orphaned_fs);
	run_test(test_fs_cleanup);
	run_test(test_fs_cleanup);
}

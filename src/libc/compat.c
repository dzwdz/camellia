#include <camellia/syscalls.h>
#include <stdio.h>
#include <camellia/compat.h>

static hid_t h = -1;
long c0_fs_wait(char *buf, long len, struct ufs_request *res) {
	if (h != -1) {
		fprintf(stderr, "c0_fs_wait: proc didn't respond to request\n");
		c0_fs_respond(NULL, -1, 0);
	}
	h = _sys_fs_wait(buf, len, res);
	return h >= 0 ? 0 : -1;
}
long c0_fs_respond(void *buf, long ret, int flags) {
	ret = _sys_fs_respond(h, buf, ret, flags);
	h = -1;
	return ret;
}

/* old syscall */
long _sys_await(void) {
	struct sys_wait2 res;
	if (_sys_wait2(-1, 0, &res) < 0) {
		return ~0;
	}
	return res.status;
}

#include <camellia/syscalls.h>
#include <stdio.h>
#include <camellia/compat.h>

#define eprintf(fmt, ...) fprintf(stderr, "user/lib/compat: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

static hid_t h = -1;
long c0_fs_wait(char *buf, long len, struct ufs_request *res) {
	if (h != -1) {
		eprintf("didn't respond to request!");
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

#include <camellia.h>
#include <camellia/syscalls.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <camellia/fs/misc.h>

_Noreturn void fs(void) {
	const size_t buflen = 1024;
	char *buf = malloc(buflen);
	if (!buf) err(1, "malloc");
	for (;;) {
		struct ufs_request req;
		hid_t reqh = ufs_wait(buf, buflen, &req);
		if (reqh < 0) errx(1, "ufs_wait error");

		switch (req.op) {
			case VFSOP_OPEN:
				printf("[logfs] open(\"%s\", 0x%x)\n", buf, req.flags);
				forward_open(reqh, buf, req.len, req.flags);
				break;
			default:
				/* Unsupported vfs operation.
				 * Currently if you never create your own file descriptors you won't receive
				 * anything but VFSOP_OPEN, but it's idiomatic to handle this anyways. */
				_sys_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
}

int main(void) {
	fs();
}

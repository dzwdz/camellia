#include "driver.h"
#include <camellia/syscalls.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void initctl_drv(handle_t killswitch) {
	struct fs_wait_response res;
	char buf[64];
	const size_t buflen = sizeof buf;
	while (!_syscall_fs_wait(buf, buflen, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				_syscall_fs_respond(NULL, res.len == 0 ? 0 : -1, 0);
				break;
			case VFSOP_WRITE:
				/* null terminate */
				if (res.len > buflen - 1)
					res.len = buflen - 1;
				buf[res.len] = '\0';
				/* cut at first whitespace */
				for (size_t i = 0; buf[i]; i++) {
					if (isspace(buf[i])) {
						buf[i] = '\0';
						break;
					}
				}
				if (!strcmp(buf, "halt")) {
					_syscall_write(killswitch, "halt", 4, 0, 0);
					exit(1);
				}
				_syscall_fs_respond(NULL, res.len, 0);
				break;
			default:
				_syscall_fs_respond(NULL, -ENOSYS, 0);
				break;
		}
	}
	exit(1);
}
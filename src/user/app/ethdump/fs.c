#include "proto.h"
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/fs/dir.h>

enum {
	H_ROOT,
	H_ETHER,
};

void fs_thread(void *arg) { (void)arg;
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		struct fs_wait_response res;
		handle_t h = _syscall_fs_wait(buf, buflen, &res);
		if (h < 0) break;
		switch (res.op) {
			long ret;
			case VFSOP_OPEN:
				ret = -1;
				if (res.len < buflen) {
					buf[res.len] = '\0';
					if (!strcmp("/", buf)) ret = H_ROOT;
					else if (!strcmp("/raw", buf)) ret = H_ETHER;
				}
				_syscall_fs_respond(h, (void*)ret, ret, 0);
				break;
			case VFSOP_READ:
				switch ((long)res.id) {
					struct dirbuild db;
					struct queue_entry *qe;
					case H_ROOT:
						dir_start(&db, res.offset, buf, sizeof buf);
						dir_append(&db, "raw");
						_syscall_fs_respond(h, buf, dir_finish(&db), 0);
						break;
					case H_ETHER:
						qe = malloc(sizeof *qe);
						qe->h = h;
						qe->next = ether_queue;
						ether_queue = qe;
						break;
					default:
						_syscall_fs_respond(h, NULL, -1, 0);
				}
				break;
			case VFSOP_WRITE:
				switch ((long)res.id) {
					case H_ETHER:
						ret = _syscall_write(state.raw_h, buf, res.len, 0, 0);
						_syscall_fs_respond(h, NULL, ret, 0);
						break;
					default:
						_syscall_fs_respond(h, NULL, -1, 0);
				}
				break;
			default:
				_syscall_fs_respond(h, NULL, -1, 0);
				break;
		}
	}
	free(buf);
}

#include <kernel/vfs/root.h>
#include <kernel/util.h>
#include <kernel/panic.h>

int vfs_root_handler(struct vfs_op_request *req) {
	switch (req->op.type) {
		case VFSOP_OPEN:
			if (req->op.open.path_len == 4
					&& !memcmp(req->op.open.path, "/tty", 4)) {
				return 0;
			}
			return -1;
		default:
			panic();
	}
}

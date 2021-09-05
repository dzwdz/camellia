#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/types.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>

int vfs_root_handler(struct vfs_op_request *req) {
	switch (req->op.type) {
		case VFSOP_OPEN:
			if (req->op.open.path_len == 4
					&& !memcmp(req->op.open.path, "/tty", 4)) {
				return 0;
			}
			return -1;
		case VFSOP_WRITE:
			switch (req->op.rw.id) {
				// every id corresponds to a special file type
				// this is a shit way to do this but :shrug:
				case 0: { // tty
					struct virt_iter iter;
					virt_iter_new(&iter, req->op.rw.buf, req->op.rw.buf_len,
							req->caller->pages, true, false);
					while (virt_iter_next(&iter))
						tty_write(iter.frag, iter.frag_len);
					return iter.prior;
				}
				default: panic();
			}
		default: panic();
	}
}

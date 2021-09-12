#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/types.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>

int vfs_root_handler(struct vfs_request *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (req->open.path_len == 4
					&& !memcmp(req->open.path, "/tty", 4)) {
				return 0;
			}
			return -1;
		case VFSOP_WRITE:
			switch (req->rw.id) {
				// every id corresponds to a special file type
				// this is a shit way to do this but :shrug:
				case 0: { // tty
					struct virt_iter iter;
					virt_iter_new(&iter, req->rw.buf, req->rw.buf_len,
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

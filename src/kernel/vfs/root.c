#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>
#include <shared/mem.h>

enum {
	HANDLE_ROOT,
	HANDLE_TTY,
};

static bool exacteq(struct vfs_request *req, const char *str) {
	int len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

int vfs_root_handler(struct vfs_request *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (exacteq(req, "/"))    return HANDLE_ROOT;
			if (exacteq(req, "/tty")) return HANDLE_TTY;
			return -1;

		case VFSOP_READ:
			switch (req->id) {
				case HANDLE_ROOT: {
					const char *src = "tty"; // TODO document directory read format
					if (req->output.len < 0) return 0; // is this needed? TODO make that a size_t or something
					int len = min((size_t) req->output.len, strlen(src) + 1);
					virt_cpy_to(req->caller->pages, req->output.buf, src, len);
					return len;
				}
				case HANDLE_TTY: {
					struct virt_iter iter;
					virt_iter_new(&iter, req->output.buf, req->output.len,
							req->caller->pages, true, false);
					while (virt_iter_next(&iter))
						tty_read(iter.frag, iter.frag_len);
					return iter.prior;
				}
				default: panic_invalid_state();
			}

		case VFSOP_WRITE:
			switch (req->id) {
				case HANDLE_ROOT: return -1;
				case HANDLE_TTY: {
					struct virt_iter iter;
					virt_iter_new(&iter, req->input.buf, req->input.len,
							req->caller->pages, true, false);
					while (virt_iter_next(&iter))
						tty_write(iter.frag, iter.frag_len);
					return iter.prior;
				}
				default: panic_invalid_state();
			}

		default: panic_invalid_state();
	}
}

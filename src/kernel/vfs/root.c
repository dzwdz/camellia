#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>
#include <shared/mem.h>

enum {
	HANDLE_ROOT,
	HANDLE_TTY,
	HANDLE_VGA,
};

static bool exacteq(struct vfs_request *req, const char *str) {
	int len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

/* truncates the length */
static void req_preprocess(struct vfs_request *req, int max_len) {
	// max_len is signed because req->*.len are signed too
	// potential place for VULNs to occur - arbitrary kernel reads etc
	if (req->offset < 0) {
		// TODO negative offsets
		req->offset = 0;
	}

	if (req->offset >= max_len) {
		req->input.len = 0;
		req->output.len = 0;
		req->offset = max_len;
		return;
	}

	if (req->input.len  < 0) req->input.len  = 0;
	if (req->output.len < 0) req->output.len = 0;

	req->input.len  = min(req->input.len,  max_len - req->offset);
	req->output.len = min(req->output.len, max_len - req->offset);

	assert(req->input.len >= 0);
	assert(req->output.len >= 0);

	assert(req->input.len + req->offset <= max_len);
	assert(req->input.len + req->offset <= max_len);
}

int vfs_root_handler(struct vfs_request *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (exacteq(req, "/"))    return HANDLE_ROOT;
			if (exacteq(req, "/tty")) return HANDLE_TTY;
			if (exacteq(req, "/vga")) return HANDLE_VGA;
			return -1;

		case VFSOP_READ:
			switch (req->id) {
				case HANDLE_ROOT: {
					// TODO document directory read format
					const char *src = "tty\0vga\0";
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
				case HANDLE_VGA: {
					char *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_to(req->caller->pages, req->output.buf,
							vga + req->offset, req->output.len);
					return req->output.len;
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
				case HANDLE_VGA: {
					void *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_from(req->caller->pages, vga + req->offset,
							req->input.buf, req->input.len);
					return req->input.len;
				}
				default: panic_invalid_state();
			}

		default: panic_invalid_state();
	}
}

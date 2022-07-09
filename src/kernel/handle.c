#include <kernel/handle.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

struct handle *handle_init(enum handle_type type) {
	struct handle *h = kmalloc(sizeof *h);
	memset(h, 0, sizeof *h);
	h->type = type;
	h->refcount = 1;
	return h;
}

void handle_close(struct handle *h) {
	if (!h) return;
	assert(h->refcount > 0);
	if (--(h->refcount) > 0) return;

	if (h->type == HANDLE_FILE) {
		vfsreq_create((struct vfs_request) {
				.type = VFSOP_CLOSE,
				.id = h->file_id,
				.caller = NULL,
				.backend = h->backend,
			});
	} else if (h->type == HANDLE_PIPE) {
		assert(!h->pipe.queued);
		if (h->pipe.sister) {
			pipe_invalidate_end(h->pipe.sister);
			h->pipe.sister->pipe.sister = NULL;
		}
	}

	if (h->backend)
		vfs_backend_refdown(h->backend);

	// TODO sanity check to check if refcount is true. handle_sanity?

	// TODO tests which would catch premature frees
	// by that i mean duplicating a handle and killing the original process
	h->type = HANDLE_INVALID;
	kfree(h);
}

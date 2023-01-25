#include <kernel/handle.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

Handle *handle_init(enum handle_type type) {
	Handle *h = kzalloc(sizeof *h);
	h->type = type;
	h->refcount = 1;
	return h;
}

void handle_close(Handle *h) {
	if (!h) return;
	assert(h->refcount > 0);
	if (--(h->refcount) > 0) return;

	if (h->type == HANDLE_FILE) {
		vfsreq_create((VfsReq) {
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
	} else if (h->type == HANDLE_FS_REQ) {
		if (h->req) vfsreq_finish_short(h->req, -1);
	}

	if (h->backend)
		vfs_backend_refdown(h->backend, true);

	// TODO sanity check to check if refcount is true. handle_sanity?

	h->type = HANDLE_INVALID;
	kfree(h);
}

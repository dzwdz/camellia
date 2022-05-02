#include <kernel/handle.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>

struct handle *handle_init(enum handle_type type) {
	struct handle *h = kmalloc(sizeof *h);
	h->type = type;
	h->refcount = 1;
	return h;
}

void handle_close(struct handle *h) {
	if (!h) return;
	assert(h->refcount > 0);
	if (--(h->refcount) > 0) return;

	if (h->type == HANDLE_FILE) {
		vfs_request_create((struct vfs_request) {
				.type = VFSOP_CLOSE,
				.id = h->file.id,
				.caller = NULL,
				.backend = h->file.backend,
			});
	}

	// TODO handle close(HANDLE_FS_FRONT)

	// TODO count allocations and frees

	// TODO sanity check to check if refcount is true. handle_sanity?

	// TODO tests which would catch premature frees
	// by that i mean duplicating a handle and killing the original process
	h->type = HANDLE_INVALID;
	kfree(h);
}

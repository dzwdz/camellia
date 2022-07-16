#pragma once
#include <kernel/proc.h>
#include <shared/flags.h>
#include <shared/types.h>
#include <stdbool.h>
#include <stddef.h>

// describes something which can act as an access function
struct vfs_backend {
	size_t refcount;
	/* references:
	 *   struct vfs_mount
	 *   struct vfs_request
	 *   struct process
	 *   struct handle
	 */
	bool heap;

	size_t potential_handlers; // 0 - orphaned
	struct vfs_request *queue;
	bool is_user;

	union {
		struct {
			struct process *handler;
		} user;
		struct {
			bool (*ready)(struct vfs_backend *);
			void (*accept)(struct vfs_request *);
		} kern;
	};
};

// describes an in-process vfs call
struct vfs_request {
	enum vfs_operation type;
	struct {
		bool kern; // if false: use .buf ; if true: use .buf_kern
		union {
			char __user *buf;
			char *buf_kern;
		};
		size_t len;
	} input;
	struct {
		char __user *buf;
		size_t len;
	} output;

	void __user *id; // handle.file.id
	long offset;
	int flags;

	struct process *caller;
	struct vfs_backend *backend;

	struct vfs_request *queue_next;
	struct vfs_request *postqueue_next; /* used by kernel backends */
	/* only one of these queues is in use at a given moment, they could
	 * be merged into a single field */
};

/** Assigns the vfs_request to the caller, and dispatches the call */
void vfsreq_create(struct vfs_request);
void vfsreq_finish(struct vfs_request*, char __user *stored, long ret, int flags, struct process *handler);

static inline void vfsreq_finish_short(struct vfs_request *req, long ret) {
	vfsreq_finish(req, (void __user *)ret, ret, 0, NULL);
}

/** Try to accept an enqueued request */
void vfs_backend_tryaccept(struct vfs_backend *);
void vfs_backend_user_accept(struct vfs_request *req);

void vfs_backend_refdown(struct vfs_backend *);


#define BACKEND_KERN(ready, accept) ((struct vfs_backend){\
	.is_user = false, \
	.heap = false, \
	.potential_handlers = 1, \
	.refcount = 1, \
	.kern = {ready, accept}})

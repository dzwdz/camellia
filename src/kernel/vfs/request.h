#pragma once
#include <camellia/types.h>
#include <stdbool.h>
#include <stddef.h>

// describes something which can act as an access function
struct vfs_backend {
	/* amount of using references
	 *  struct vfs_mount
	 *  struct vfs_request
	 *  struct handle
	 * once it reaches 0, it'll never increase */
	size_t usehcnt; /* struct vfs_mount */
	/* amount of providing references
	 *  struct process
	 * 0 - orphaned, will never increase */
	size_t provhcnt;

	struct vfs_request *queue;
	bool is_user;
	union {
		struct {
			struct process *handler;
		} user;
		struct {
			void (*accept)(struct vfs_request *);
			void (*cleanup)(struct vfs_backend *);
			void *data;
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

	// TODO why doesn't this just have a reference to the handle?

	void __user *id; // handle.file.id
	long offset;
	int flags;

	/* if caller != NULL, owned by it - don't free, the allocation will be reused
	 * if caller == NULL, free on finish */
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

// TODO the bool arg is confusing. maybe this should just be a function
// that verified the refcount and potentially frees the backend
void vfs_backend_refdown(struct vfs_backend *, bool use);

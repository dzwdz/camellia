#pragma once
#include <kernel/types.h>
#include <stdbool.h>
#include <stddef.h>

/* describes something which can act as an access function */
struct VfsBackend {
	/* amount of using references
	 *  VfsMount
	 *  VfsReq
	 *  Handle
	 * once it reaches 0, it'll never increase */
	size_t usehcnt; /* VfsMount */
	/* amount of providing references
	 *  Proc
	 * 0 - orphaned, will never increase */
	size_t provhcnt;

	VfsReq *queue;
	bool is_user;
	union {
		struct {
			Proc *handler;
		} user;
		struct {
			void (*accept)(VfsReq *);
			void (*cleanup)(VfsBackend *);
			void *data;
		} kern;
	};
};

/* describes an in-progress vfs call */
struct VfsReq {
	enum vfs_op type;
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
	Proc *caller;
	VfsBackend *backend;

	VfsReq *queue_next;
	VfsReq *postqueue_next; /* used by kernel backends */
	/* only one of these queues is in use at a given moment, they could
	 * be merged into a single field */
};

/** Assigns the vfs_request to the caller, and dispatches the call */
void vfsreq_create(VfsReq);
void vfsreq_finish(VfsReq*, char __user *stored, long ret, int flags, Proc *handler);

static inline void vfsreq_finish_short(VfsReq *req, long ret) {
	vfsreq_finish(req, (void __user *)ret, ret, 0, NULL);
}

/** Try to accept an enqueued request */
void vfs_backend_tryaccept(VfsBackend *);

// TODO the bool arg is confusing. maybe this should just be a function
// that verified the refcount and potentially frees the backend
void vfs_backend_refdown(VfsBackend *, bool use);

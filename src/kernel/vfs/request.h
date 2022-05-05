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

	size_t potential_handlers; // 0 - orphaned
	struct vfs_request *queue;
	bool is_user;

	union {
		struct {
			struct process *handler;
		} user;
		struct {
			bool (*ready)(struct vfs_backend *);

			// return value might be passed to caller
			// TODO make return void
			int (*accept)(struct vfs_request *);
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

	int id; // handle.file.id
	int offset;

	struct process *caller;
	struct vfs_backend *backend;

	struct vfs_request *queue_next;
};

/** Assigns the vfs_request to the caller, and dispatches the call */
int vfsreq_create(struct vfs_request);
int vfsreq_finish(struct vfs_request *, int ret);

/** Try to accept an enqueued request
 * @return same as _syscall_fs_wait, passed to it. except on calls to kern backend, where it returns the result of the fs op - also gets directly passed to caller. it's a mess */
// TODO fix the return value mess
int vfs_backend_tryaccept(struct vfs_backend *);
int vfs_backend_user_accept(struct vfs_request *req);

void vfs_backend_refdown(struct vfs_backend *);

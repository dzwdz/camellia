#pragma once
#include <shared/flags.h>

enum vfs_backend_type {
	VFS_BACK_ROOT,
	VFS_BACK_USER,
};

// describes something which can act as an access function
struct vfs_backend {
	enum vfs_backend_type type;

	// only used with VFS_BACK_USER
	struct process *handler;
	struct process *queue;
};

// describes an in-process vfs call
struct vfs_request {
	enum vfs_operation type;
	struct { // TODO maybe this should be a separate type
		bool kern; // if false: use .buf ; if true: use .buf_kern
		union {
			char __user *buf;
			char *buf_kern;
		};
		int len;
	} input;
	struct {
		bool kern; // if false: use .buf ; if true: use .buf_kern
		union {
			char __user *buf;
			char *buf_kern;
		};
		int len;
	} output;

	int id; // handle.file.id

	struct process *caller;
	struct vfs_backend *backend;
};

/** Assigns the vfs_request to the caller, and calls the backend. Might not
 * return - can switch processes! */
int vfs_request_create(struct vfs_request);
_Noreturn void vfs_request_pass2handler(struct vfs_request *);
int vfs_request_finish(struct vfs_request *, int ret);

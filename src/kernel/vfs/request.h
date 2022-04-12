#pragma once
#include <shared/flags.h>
#include <shared/types.h>
#include <stdbool.h>

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
};

/** Assigns the vfs_request to the caller, and dispatches the call */
int vfs_request_create(struct vfs_request);
int vfs_request_accept(struct vfs_request *);
int vfs_request_finish(struct vfs_request *, int ret);

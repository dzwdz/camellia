#pragma once

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

enum vfs_operation {
	VFSOP_OPEN,
	VFSOP_WRITE,
};

// describes an in-process vfs call
struct vfs_request {
	enum vfs_operation type;
	union {
		struct {
			char *path;
			int path_len;
		} open;
		struct {
			char __user *buf;
			int buf_len;
			int id; // filled in by the kernel
		} rw;
	};

	struct process *caller;
	struct vfs_backend *backend;
};

_Noreturn void vfs_request_create(struct vfs_request);
_Noreturn void vfs_request_pass2handler(struct vfs_request *);
_Noreturn void vfs_request_finish(struct vfs_request *, int ret);

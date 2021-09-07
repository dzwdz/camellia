#pragma once
#include <shared/vfs.h>

enum vfs_backend_type {
	VFS_BACK_ROOT,
	VFS_BACK_USER,
};

// describes something which can acts as an access function
struct vfs_backend {
	enum vfs_backend_type type;
};

// describes an in-progress vfs call
struct vfs_op_request {
	struct vfs_op op;
	struct process *caller;
	struct vfs_backend *backend;
};


// these can switch processes
_Noreturn void vfs_backend_dispatch(struct vfs_backend *backend, struct vfs_op op);
_Noreturn void vfs_backend_finish(struct vfs_op_request *, int ret);

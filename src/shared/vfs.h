// requires the user_ptr type from kernel/types.h or init/types.h
// TODO add some macro magic which prints an error when it isn't defined

#pragma once

enum vfs_op_types {
	VFSOP_OPEN,
	VFSOP_WRITE,
};

/* currently, this struct is fully created in kernelspace
 * i might add a syscall which allows a process to just pass it raw, with some
 * validation - so keep that in mind */
struct vfs_op {
	enum vfs_op_types type;
	union {
		struct {
			const char *path;
			int path_len;
		} open;
		struct {
			user_ptr buf;
			int buf_len;
			int id; // filled in by the kernel
		} rw;
	};
};

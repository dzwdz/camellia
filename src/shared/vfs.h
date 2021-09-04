// requires the user_ptr type from kernel/types.h or init/types.h
// TODO add some macro magic which prints an error when it isn't defined

#pragma once

enum vfs_op_types {
	VFSOP_OPEN,
};

struct vfs_op {
	enum vfs_op_types type;
	union {
		struct {
			const char *path;
			int path_len;
		} open;
	};
};

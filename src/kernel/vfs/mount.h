#pragma once
#include <kernel/handle.h>

struct vfs_mount {
	struct vfs_mount *prev;
	char *prefix;
	size_t prefix_len;
};

// prepares init's filesystem view
struct vfs_mount *vfs_mount_seed(void);
struct vfs_mount *vfs_mount_resolve(
		struct vfs_mount *top, const char *path, size_t path_len);


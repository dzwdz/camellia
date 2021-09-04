#pragma once
#include <kernel/vfs/backend.h>
#include <stddef.h>

struct vfs_mount {
	struct vfs_mount *prev;
	char *prefix;
	size_t prefix_len;
	struct vfs_backend *backend;
};

// prepares init's filesystem view
struct vfs_mount *vfs_mount_seed(void);
struct vfs_mount *vfs_mount_resolve(
		struct vfs_mount *top, const char *path, size_t path_len);

#pragma once
#include <kernel/vfs/request.h>
#include <stddef.h>

struct vfs_mount {
	struct vfs_mount *prev;
	char *prefix;
	size_t prefix_len;
	bool prefix_owned;
	struct vfs_backend *backend;
	size_t refs; /* counts all references, atm from:
	              *  - struct vfs_mount
	              *  - struct proc
	              */
};

// prepares init's filesystem view
struct vfs_mount *vfs_mount_seed(void);
struct vfs_mount *vfs_mount_resolve(
		struct vfs_mount *top, const char *path, size_t path_len);
/** Decrements the reference count, potentially freeing the mount. */
void vfs_mount_remref(struct vfs_mount *mnt);

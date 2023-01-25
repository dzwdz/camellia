#pragma once
#include <kernel/types.h>
#include <kernel/vfs/request.h>
#include <stddef.h>

struct VfsMount {
	VfsMount *prev;
	const char *prefix;
	size_t prefix_len;
	bool prefix_owned;
	VfsBackend *backend;
	size_t refs; /* counts all references, atm from:
	              *  - VfsMount
	              *  - Proc
	              */
};

// prepares init's filesystem view
VfsMount *vfs_mount_seed(void);
VfsMount *vfs_mount_resolve(
		VfsMount *top, const char *path, size_t path_len);
/** Decrements the reference count, potentially freeing the mount. */
void vfs_mount_remref(VfsMount *mnt);

void vfs_root_register(const char *path, void (*accept)(VfsReq *));

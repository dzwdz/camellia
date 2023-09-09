#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>

static VfsMount *mount_root = NULL;

VfsMount *vfs_mount_seed(void) {
	return mount_root;
}

void vfs_root_register(const char *path, void (*accept)(VfsReq *)) {
	VfsBackend *backend = kmalloc(sizeof *backend);
	VfsMount *mount = kmalloc(sizeof *mount);
	*backend = (VfsBackend) {
		.is_user = false,
		.usehcnt = 1,
		.provhcnt = 1,
		.kern.accept = accept,
	};
	*mount = (VfsMount){
		.prev = mount_root,
		.prefix = path,
		.prefix_len = strlen(path),
		.backend = backend,
		.refs = 1,
	};
	mount_root = mount;
}


VfsMount *vfs_mount_resolve(
		VfsMount *top, const char *path, size_t path_len)
{
	for (; top; top = top->prev) {
		if (top->prefix_len > path_len)
			continue;
		if (memcmp(top->prefix, path, top->prefix_len) != 0)
			continue;

		/* ensure that there's no garbage after the match
		 * e.g. don't recognize /thisasdf as mounted under /this */

		if (top->prefix_len == path_len) // can't happen if prefix == path
			break;
		if (path[top->prefix_len] == '/')
			break;

		/* Also valid if prefix ends with '/'. Can only happen with kernel-
		 * provided mounts. */
		if (top->prefix_len != 0 && path[top->prefix_len-1] == '/')
			break;
	}
	return top;
}

void vfs_mount_remref(VfsMount *mnt) {
	assert(mnt);
	assert(mnt->refs > 0);
	if (--(mnt->refs) > 0) return;

	VfsMount *prev = mnt->prev;
	if (mnt->backend) {
		vfs_backend_refdown(mnt->backend, true);
	}
	if (mnt->prefix_owned) {
		kfree((void*)mnt->prefix);
	}
	kfree(mnt);

	if (prev) {
		vfs_mount_remref(prev);
	}
}

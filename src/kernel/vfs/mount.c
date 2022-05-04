#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>

struct vfs_mount *vfs_mount_seed(void) {
	struct vfs_mount   *mount   = kmalloc(sizeof *mount);
	struct vfs_backend *backend = kmalloc(sizeof *backend);
	backend->type = VFS_BACK_ROOT;
	backend->potential_handlers = 1;
	backend->refcount = 1;
	*mount = (struct vfs_mount){
		.prev = NULL,
		.prefix = NULL,
		.prefix_len = 0,
		.backend = backend,
		.refs = 1, // never to be freed
	};
	return mount;
}

struct vfs_mount *vfs_mount_resolve(
		struct vfs_mount *top, const char *path, size_t path_len)
{
	for (; top; top = top->prev) {
		if (top->prefix_len > path_len)
			continue;
		if (memcmp(top->prefix, path, top->prefix_len) != 0)
			continue;

		/* ensure that there's no garbage after the match
		 * recognize that e.g. /thisasdf doesn't get recognized as mounted under
		 * /this */
		
		if (top->prefix_len == path_len) // can't happen if prefix == path
			break;
		if (path[top->prefix_len] == '/')
			break;
	}
	return top;
}

void vfs_mount_remref(struct vfs_mount *mnt) {
	assert(mnt);
	assert(mnt->refs > 0);
	if (--(mnt->refs) > 0) return;

	struct vfs_mount *prev = mnt->prev;
	if (mnt->backend)
		vfs_backend_refdown(mnt->backend);
	kfree(mnt->prefix);
	kfree(mnt);

	if (prev) vfs_mount_remref(prev);
}

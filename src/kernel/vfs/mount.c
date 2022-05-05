#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>

// TODO not the place where this should be done
static struct vfs_mount *mount_root = NULL;

struct vfs_mount *vfs_mount_seed(void) {
	return mount_root;
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
	if (mnt->prefix_owned)
		kfree((void*)mnt->prefix);
	kfree(mnt);

	if (prev) vfs_mount_remref(prev);
}

void vfs_mount_root_register(const char *path, struct vfs_backend *backend) {
	struct vfs_mount *mount = kmalloc(sizeof *mount);
	*mount = (struct vfs_mount){
		.prev = mount_root,
		.prefix = path,
		.prefix_len = strlen(path),
		.backend = backend,
		.refs = 1,
	};
	mount_root = mount;
}

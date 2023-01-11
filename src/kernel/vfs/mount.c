#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>

static struct vfs_mount *mount_root = NULL;

struct vfs_mount *vfs_mount_seed(void) {
	return mount_root;
}

void vfs_root_register(const char *path, void (*accept)(struct vfs_request *)) {
	struct vfs_backend *backend = kmalloc(sizeof *backend);
	struct vfs_mount *mount = kmalloc(sizeof *mount);
	*backend = (struct vfs_backend) {
		.is_user = false,
		.usehcnt = 1,
		.provhcnt = 1,
		.kern.accept = accept,
	};
	*mount = (struct vfs_mount){
		.prev = mount_root,
		.prefix = path,
		.prefix_len = strlen(path),
		.backend = backend,
		.refs = 1,
	};
	mount_root = mount;
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
		 * e.g. don't recognize /thisasdf as mounted under /this */

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

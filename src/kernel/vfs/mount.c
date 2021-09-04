#include <kernel/mem.h>
#include <kernel/util.h>
#include <kernel/vfs/mount.h>

struct vfs_mount *vfs_mount_seed(void) {
	struct vfs_mount   *mount   = kmalloc(sizeof(struct vfs_mount));
	struct vfs_backend *backend = kmalloc(sizeof(struct vfs_backend));
	backend->type = VFS_BACK_ROOT;
	*mount = (struct vfs_mount){
		.prev = NULL,
		.prefix = "",
		.prefix_len = 0,
		.backend = backend,
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

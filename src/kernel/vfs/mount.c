#include <kernel/mem.h>
#include <kernel/vfs/mount.h>

struct vfs_mount *vfs_mount_seed(void) {
	struct vfs_mount *mount = kmalloc(sizeof(struct vfs_mount));
	*mount = (struct vfs_mount){
		.prev = NULL,
		.prefix = "/tty",
		.prefix_len = 4,
		.fd = {
			.type = FD_SPECIAL_TTY,
		},
	};
	return mount;
}

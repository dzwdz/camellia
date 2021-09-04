#pragma once
#include <kernel/vfs/mount.h>

// the VFS provided to init
int vfs_root_handler(struct vfs_op_request *);

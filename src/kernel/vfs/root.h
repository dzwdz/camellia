#pragma once
#include <kernel/vfs/request.h>

// the VFS provided to init
int vfs_root_handler(struct vfs_request *);

#pragma once
#include <kernel/vfs/request.h>

void vfs_root_accept(struct vfs_request *);
bool vfs_root_ready(struct vfs_backend *);

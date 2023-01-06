#pragma once
#include <kernel/vfs/request.h>

struct vfs_backend *procfs_backend(struct process *proc);

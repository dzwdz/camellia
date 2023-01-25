#pragma once
#include <kernel/vfs/request.h>

VfsBackend *procfs_backend(Proc *proc);

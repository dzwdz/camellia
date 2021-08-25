#pragma once
#include <kernel/fd.h>

struct vfs_mount {
	struct vfs_mount *prev;
	char *prefix;
	size_t prefix_len;
	struct fd fd;
};

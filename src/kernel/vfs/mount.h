#pragma once

struct vfs_mount {
	struct vfs_mount *prev;
	char *prefix;
	size_t prefix_len;
};

#pragma once

enum vfs_operation {
	VFSOP_OPEN,
	VFSOP_WRITE,
};

enum {
	MEMFLAG_PRESENT = 1 << 0,
};

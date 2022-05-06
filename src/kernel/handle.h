#pragma once

enum handle_type; // forward declaration for proc.h

#include <kernel/vfs/mount.h>
#include <shared/types.h>
#include <stddef.h>

enum handle_type {
	HANDLE_INVALID = 0,
	HANDLE_FILE,

	HANDLE_FS_FRONT,
};

struct handle {
	enum handle_type type;
	struct vfs_backend *backend;
	int file_id; // only applicable to HANDLE_FILE

	size_t refcount;
};

struct handle *handle_init(enum handle_type);
void handle_close(struct handle *);

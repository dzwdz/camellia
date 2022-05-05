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
	union {
		// TODO consolidate backend fields
		struct {
			struct vfs_backend *backend;
			int id;
		} file;
		struct {
			struct vfs_backend *backend;
		} fs;
	};

	size_t refcount;
};

struct handle *handle_init(enum handle_type);
void handle_close(struct handle *);

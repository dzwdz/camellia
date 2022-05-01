#pragma once
#include <kernel/vfs/mount.h>
#include <shared/types.h>
#include <stddef.h>

#define HANDLE_MAX 16

enum handle_type {
	HANDLE_INVALID = 0,
	HANDLE_FILE,

	HANDLE_FS_FRONT,
};

struct handle {
	enum handle_type type;
	union {
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

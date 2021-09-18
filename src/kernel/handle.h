#pragma once
#include <kernel/vfs/mount.h>
#include <shared/types.h>
#include <stddef.h>

#define HANDLE_MAX 16

enum handle_type {
	HANDLE_EMPTY,
	HANDLE_FILE,

	HANDLE_FS_FRONT,
	HANDLE_FS_BACK,
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
};

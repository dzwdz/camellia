#pragma once
#include <kernel/types.h>
#include <kernel/vfs/mount.h>
#include <stddef.h>

#define HANDLE_MAX 16

typedef int handle_t; // TODO duplicated in syscalls.h

enum handle_type {
	HANDLE_EMPTY,
	HANDLE_FILE,
};

struct handle {
	enum handle_type type;
	union {
		struct {
			struct vfs_backend *backend;
			int id;
		} file;
	};
};

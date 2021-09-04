#pragma once
#include <kernel/types.h>
#include <stddef.h>

#define HANDLE_MAX 16

typedef int handle_t; // TODO duplicated in syscalls.h

enum handle_type {
	HANDLE_EMPTY,
	HANDLE_SPECIAL_TTY,
};

struct handle {
	enum handle_type type;
};


enum handleop { // describes the operations which can be done on handles
	HANDLEOP_MOUNT, // also closes the original handle
	HANDLEOP_OPEN,

	HANDLEOP_READ,
	HANDLEOP_WRITE,
	HANDLEOP_CLOSE,
};

struct handleop_args {
	enum handleop type;
	struct handle *handle;
	union {
		struct { // HANDLEOP_MOUNT
			struct mount *target;
		} mnt;
		struct { // HANDLEOP_OPEN
			struct handle *target;
			const char *path; // relative to the mount point
			size_t len;
		} open;
		struct { // HANDLEOP_READ, HANDLEOP_WRITE
			user_ptr ptr;
			size_t len;
		} rw;
	};
};

int handleop_dispatch(struct handleop_args args);

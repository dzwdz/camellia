#pragma once
#include <kernel/types.h>
#include <stddef.h>

#define FD_MAX 16

typedef int fd_t; // TODO duplicated in syscalls.h

enum fd_type {
	FD_EMPTY,
	FD_SPECIAL_TTY,
};

struct fd {
	enum fd_type type;
};


enum fdop { // describes the operations which can be done on file descriptors
	FDOP_MOUNT, // also closes the original fd
	FDOP_OPEN,  // when the file descriptor is mounted, open a file relative to it

	FDOP_READ,
	FDOP_WRITE,
	FDOP_CLOSE,
};

struct fdop_args {
	enum fdop type;
	struct fd *fd;
	union {
		struct { // FDOP_MOUNT
			struct mount *target;
		} mnt;
		struct { // FDOP_OPEN
			struct fd *target;
			const char *path; // relative to the mount point
			size_t len;
		} open;
		struct { // FDOP_READ, FDOP_WRITE
			user_ptr ptr;
			size_t len;
		} rw;
	};
};

int fdop_dispatch(struct fdop_args args);

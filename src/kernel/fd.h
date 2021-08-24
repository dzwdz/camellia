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
	FDOP_READ,
	FDOP_WRITE,
	FDOP_CLOSE,
};

int fdop_dispatch(enum fdop fdop, struct fd *fd, user_ptr ptr, size_t len);

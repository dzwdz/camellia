#pragma once
#include <errno.h> // only for ENOSYS

#define TIOCGWINSZ 0
struct winsize {
	int ws_row, ws_col;
};

static inline int ioctl(int fd, int req, ...) {
	(void)fd; (void)req;
	errno = ENOSYS;
	return -1;
}

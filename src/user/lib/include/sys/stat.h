#pragma once
#include <sys/types.h>
#include <errno.h> // only for ENOSYS

#define S_ISFIFO(x) 0

struct stat {};
static inline int fstat(int fd, struct stat *sb) {
	(void)fd; (void)sb;
	errno = ENOSYS;
	return -1;
}

int mkdir(const char *path, mode_t mode);

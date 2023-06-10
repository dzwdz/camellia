#include <bits/panic.h>
#include <errno.h>
#include <fcntl.h>

#include <stdio.h>

int open(const char *path, int flags, ...) {
	(void)path; (void)flags;
	_klogf("failing open(\"%s\")", path);
	return errno = ENOSYS, -1;
}

int fcntl(int fd, int cmd, ...) {
	(void)fd; (void)cmd;
	_klogf("failing fcntl(%d)", cmd);
	return errno = ENOSYS, -1;
}

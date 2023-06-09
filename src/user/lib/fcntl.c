#include <bits/panic.h>
#include <fcntl.h>

int open(const char *path, int flags, ...) {
	(void)path; (void)flags;
	__libc_panic("unimplemented");
}

int fcntl(int fd, int cmd, ...) {
	(void)fd; (void)cmd;
	__libc_panic("unimplemented");
}

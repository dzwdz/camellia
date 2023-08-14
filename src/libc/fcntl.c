#include <bits/panic.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#include <stdio.h>

int open(const char *path, int flags, ...) {
	(void)path; (void)flags;
	_klogf("failing open(\"%s\")", path);
	return errno = ENOSYS, -1;
}

int fcntl(int fd, int cmd, ...) {
	va_list argp;
	va_start(argp, cmd);
	if (cmd == F_DUPFD) {
		int to = va_arg(argp, int);
		va_end(argp);
		return _sys_dup(fd, to, DUP_SEARCH);
	} else {
		va_end(argp);
		_klogf("failing fcntl(%d)", cmd);
		return errno = ENOSYS, -1;
	}
}

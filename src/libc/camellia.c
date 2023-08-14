#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

hid_t camellia_open(const char *path, int flags) {
	hid_t ret;
	char *buf;
	size_t len;

	if (path == NULL)
		return errno = EINVAL, -EINVAL;
	if (flags & OPEN_CREATE)
		flags |= OPEN_WRITE;

	len = absolutepath(NULL, path, 0);
	buf = malloc(len);
	if (!buf)
		return -errno;
	absolutepath(buf, path, len);
	ret = _sys_open(buf, strlen(buf), flags);
	free(buf);

	if (ret < 0)
		errno = -ret;

	return ret;
}

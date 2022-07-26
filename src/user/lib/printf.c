#include <camellia/syscalls.h>
#include <shared/printf.h>
#include <stdio.h>
#include <string.h>


static void backend_file(void *arg, const char *buf, size_t len) {
	fwrite(buf, 1, len, (FILE *)arg);
}

int printf(const char *fmt, ...) {
	int ret = 0;
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_file, (void*)stdout);
	va_end(argp);
	return ret;
}

static void backend_buf(void *arg, const char *buf, size_t len) {
	char **ptrs = arg;
	size_t space = ptrs[1] - ptrs[0];
	if (len > space) len = space;
	memcpy(ptrs[0], buf, len);
	ptrs[0] += len;
}

int snprintf(char *str, size_t len, const char *fmt, ...) {
	int ret = 0;
	char *ptrs[2] = {str, str + len};
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_buf, &ptrs);
	va_end(argp);
	if (ptrs[0] < ptrs[1]) *ptrs[0] = '\0';
	return ret;
}

int _klogf(const char *fmt, ...) {
	// idiotic. however, this hack won't matter anyways
	char buf[256];
	int ret = 0;
	char *ptrs[2] = {buf, buf + sizeof buf};
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_buf, &ptrs);
	va_end(argp);
	if (ptrs[0] < ptrs[1]) *ptrs[0] = '\0';
	_syscall_debug_klog(buf, ret);
	return ret;
}

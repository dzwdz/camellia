#include <camellia/syscalls.h>
#include <shared/printf.h>
#include <stdio.h>
#include <string.h>


static void backend_file(void *arg, const char *buf, size_t len) {
	fwrite(buf, 1, len, arg);
}

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap) {
	return __printf_internal(fmt, ap, backend_file, f);
}

static void backend_buf(void *arg, const char *buf, size_t len) {
	char **ptrs = arg;
	size_t space = ptrs[1] - ptrs[0];
	if (len > space) len = space;

	memcpy(ptrs[0], buf, len);
	ptrs[0] += len;
	/* ptrs[1] is the last byte of the buffer, it must be 0.
	 * on overflow:
	 *   ptrs[0] + (ptrs[1] - ptrs[0]) = ptrs[1] */
	*ptrs[0] = '\0';
}

int vsnprintf(char *restrict str, size_t len, const char *restrict fmt, va_list ap) {
	char *ptrs[2] = {str, str + len - 1};
	return __printf_internal(fmt, ap, backend_buf, &ptrs);
}


int printf(const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vprintf(fmt, argp);
	va_end(argp);
	return ret;
}

int fprintf(FILE *restrict f, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vfprintf(f, fmt, argp);
	va_end(argp);
	return ret;
}

int snprintf(char *restrict str, size_t len, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vsnprintf(str, len, fmt, argp);
	va_end(argp);
	return ret;
}

int vprintf(const char *restrict fmt, va_list ap) {
	return vfprintf(stdout, fmt, ap);
}

int _klogf(const char *fmt, ...) {
	char buf[256];
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vsnprintf(buf, sizeof buf, fmt, argp);
	va_end(argp);
	_syscall_debug_klog(buf, ret);
	return ret;
}

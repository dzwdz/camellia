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

int sprintf(char *restrict s, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vsnprintf(s, ~0, fmt, argp);
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
	_sys_debug_klog(buf, ret);
	return ret;
}

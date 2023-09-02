#include <shared/printf.h>
#include <stdio.h>

static void backend_file(void *arg, const char *buf, size_t len) {
	fwrite(buf, 1, len, arg);
}

int fprintf(FILE *restrict f, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vfprintf(f, fmt, argp);
	va_end(argp);
	return ret;
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

int vprintf(const char *restrict fmt, va_list ap) {
	return vfprintf(stdout, fmt, ap);
}

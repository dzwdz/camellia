#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <shared/printf.h>
#include <stdio.h>

int sprintf(char *restrict s, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vsnprintf(s, ~0, fmt, argp);
	va_end(argp);
	return ret;
}

int vsprintf(char *restrict s, const char *restrict fmt, va_list ap) {
	return vsnprintf(s, ~0, fmt, ap);
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

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Noreturn void err(int ret, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vwarn(fmt, args);
	va_end(args);
	exit(ret);
}

_Noreturn void errx(int ret, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vwarnx(fmt, args);
	va_end(args);
	exit(ret);
}

void vwarn(const char *fmt, va_list args) {
	fprintf(stderr, "%s: ", getprogname());
	if (fmt) {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, ": ");
	}
	fprintf(stderr, "%s\n", strerror(errno));
}

void vwarnx(const char *fmt, va_list args) {
	fprintf(stderr, "%s: ", getprogname());
	if (fmt) vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
}

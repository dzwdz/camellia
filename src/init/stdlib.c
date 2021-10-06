#include <init/stdlib.h>
#include <shared/syscalls.h>
#include <stdarg.h>

int memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *c1 = s1, *c2 = s2;
	for (size_t i = 0; i < n; i++) {
		if (c1[i] != c2[i]) {
			if (c1[i] < c2[i])  return -1;
			else                return 1;
		}
	}
	return 0;
}

size_t strlen(const char *s) {
	size_t c = 0;
	while (*s++) c++;
	return c;
}

int printf(const char *fmt, ...) {
	const char *seg = fmt; // beginning of the current segment
	int total = 0;
	va_list argp;
	va_start(argp, fmt);
	for (;;) {
		char c = *fmt++;
		switch (c) {
			case '\0':
				// TODO don't assume that stdout is @ fd 0
				_syscall_write(0, seg, fmt - seg - 1, 0);
				return total + (fmt - seg - 1);

			case '%':
				_syscall_write(0, seg, fmt - seg - 1, 0);
				total += fmt - seg - 1;
				c = *fmt++;
				switch (c) {
					case 's':
						const char *s = va_arg(argp, char*);
						_syscall_write(0, s, strlen(s), 0);
						total += strlen(s);
						break;
				}
				seg = fmt;
				break;
		}
	}
}

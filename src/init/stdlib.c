#include <init/stdlib.h>
#include <shared/syscalls.h>
#include <stdarg.h>

int __tty_fd;


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
				size_t pad_len = 0;
parse_fmt:
				c = *fmt++;
				switch (c) {
					case '0':
						pad_len = *fmt++ - '0'; // can skip over the null byte, idc
						goto parse_fmt;

					case 'c':
						char c = va_arg(argp, int);
						_syscall_write(0, &c, 1, 0);
						total += 1;
						break;

					case 's':
						const char *s = va_arg(argp, char*);
						if (s) {
							_syscall_write(0, s, strlen(s), 0);
							total += strlen(s);
						}
						break;

					case 'x':
						unsigned int n = va_arg(argp, int);
						size_t i = 4; // nibbles * 4
						while (n >> i && i < (sizeof(int) * 8))
							i += 4;

						if (i < pad_len * 4)
							i = pad_len * 4;

						while (i > 0) {
							i -= 4;
							char h = '0' + ((n >> i) & 0xf);
							if (h > '9') h += 'a' - '9' - 1;
							_syscall_write(0, &h, 1, 0);
							total++;
						}
						break;
				}
				seg = fmt;
				break;
		}
	}
}

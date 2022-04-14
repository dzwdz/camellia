#include <shared/mem.h>
#include <shared/printf.h>

int __printf_internal(const char *fmt, va_list argp,
		void (*back)(void*, const char*, size_t), void *back_arg)
{
	const char *seg = fmt; // beginning of the current segment
	int total = 0;

	for (;;) {
		char c = *fmt++;
		switch (c) {
			case '\0':
				back(back_arg, seg, fmt - seg - 1);
				return total + (fmt - seg - 1);

			case '%':
				back(back_arg, seg, fmt - seg - 1);
				total += fmt - seg - 1;
				size_t pad_len = 0;

				c = *fmt++;
				while (1) {
					switch (c) {
						case '0':
							pad_len = *fmt++ - '0'; // can skip over the null byte, idc
							break;
						default:
							goto modifier_break;
					}
					c = *fmt++;
				}
modifier_break:
				switch (c) {
					case 'c':
						char c = va_arg(argp, int);
						back(back_arg, &c, 1);
						total += 1;
						break;

					case 's':
						const char *s = va_arg(argp, char*);
						if (s) {
							back(back_arg, s, strlen(s));
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
							back(back_arg, &h, 1);
							total++;
						}
						break;
				}
				seg = fmt;
				break;
		}
	}
}

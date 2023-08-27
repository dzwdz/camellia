#include <shared/mem.h>
#include <shared/printf.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

enum lenmod {
	LM_int,
	LM_long,
	LM_longlong,
	LM_size,
};

struct out_state {
	void (*back)(void *, const char *, size_t);
	void *backarg;
	int written;

	char *cache;
	size_t cpos, clen;
};

struct mods {
	char fill_char;
	size_t field_width;
	size_t precision;
};


static int flush(struct out_state *os) {
	if (os->cpos) {
		os->back(os->backarg, os->cache, os->cpos);
		os->written += os->cpos;
		os->cpos = 0;
	}
	return os->written;
}

static void output(struct out_state *os, const char *buf, size_t len) {
	if (os->cpos + len < os->clen) {
		memcpy(os->cache + os->cpos, buf, len);
		os->cpos += len;
		return;
	}
	flush(os);
	os->back(os->backarg, buf, len);
	os->written += len;
}

static void output_c(struct out_state *os, char c, int amt) {
	for (int i = 0; i < amt; i++)
		output(os, &c, 1);
}


static void pad(struct out_state *os, struct mods *m, size_t len) {
	output_c(os, m->fill_char, m->field_width - len);
}

static void padnum(struct out_state *os, struct mods *m, size_t len, char sign) {
	if (len < m->precision) {
		output_c(os, m->fill_char, m->field_width - m->precision - (sign ? 1 : 0));
		if (sign) output_c(os, sign, 1);
		output_c(os, '0', m->precision - len);
	} else {
		output_c(os, m->fill_char, m->field_width - len - (sign ? 1 : 0));
		if (sign) output_c(os, sign, 1);
	}
}

static void output_uint(struct out_state *os, struct mods *m, unsigned long long n, char sign) {
	char buf[sizeof(unsigned long long) * 3];
	size_t pos = sizeof(buf);

	if (!n) {
		buf[--pos] = '0';
	} else while (n) {
		unsigned long long q = n / 10, r = n % 10;
		buf[--pos] = r + '0';
		n = q;
	}
	size_t len = sizeof(buf) - pos;
	padnum(os, m, len, sign);
	output(os, buf + pos, len);
}

static void output_uint16(struct out_state *os, struct mods *m, unsigned long long n) {
	size_t len = 1;
	while (n >> (len * 4) && (len * 4) < (sizeof(n) * 8))
		len++;
	padnum(os, m, len, '\0');
	while (len-- > 0) {
		char h = '0' + ((n >> (len * 4)) & 0xf);
		if (h > '9') h += 'a' - '9' - 1;
		output_c(os, h, 1);
	}
}

static void output_octal(struct out_state *os, struct mods *m, unsigned long long n) {
	char buf[sizeof(unsigned long long) * 3];
	size_t pos = sizeof(buf);

	if (!n) {
		buf[--pos] = '0';
	} else while (n) {
		unsigned long long q = n / 8, r = n % 8;
		buf[--pos] = r + '0';
		n = q;
	}
	size_t len = sizeof(buf) - pos;
	padnum(os, m, len, '\0');
	output(os, buf + pos, len);
}


int __printf_internal(const char *fmt, va_list argp,
		void (*back)(void*, const char*, size_t), void *backarg)
{
	const char *seg = fmt; /* beginning of the current non-modifier streak */
	char cache[64];
	struct out_state os = {
		.back = back,
		.backarg = backarg,
		.cache = cache,
		.cpos = 0,
		.clen = sizeof(cache),
	};

	for (;;) {
		char c = *fmt++;
		if (c == '\0') {
			output(&os, seg, fmt - seg - 1);
			return flush(&os);
		}
		if (c != '%') continue;
		output(&os, seg, fmt - seg - 1);

		struct mods m = {
			.fill_char = ' ',
			.field_width = 0,
			.precision = 0,
		};

		for (bool modifier = true; modifier;) {
			c = *fmt++;
			switch (c) {
				case '0':
					m.fill_char = '0';
					break;
				default:
					modifier = false;
					break;
			}
		}

		while ('0' <= c && c <= '9') {
			m.field_width *= 10;
			m.field_width += c - '0';
			c = *fmt++;
		}

		if (c == '.') {
			c = *fmt++;
			if (c == '*') {
				// TODO handle negative precision
				m.precision = va_arg(argp, int);
				c = *fmt++;
			} else while ('0' <= c && c <= '9') {
				m.precision *= 10;
				m.precision += c - '0';
				c = *fmt++;
			}
		}

		enum lenmod lm;
		switch (c) {
			case 'l':
				lm = LM_long;
				c = *fmt++;
				if (c == 'l') {
					lm = LM_longlong;
					c = *fmt++;
				}
				break;
			case 'z':
				lm = LM_size;
				c = *fmt++;
				break;
			case 'j':
				lm = LM_longlong;
				c = *fmt++;
				break;
			default:
				lm = LM_int;
				break;
		}

		switch (c) {
			unsigned long n, len;
			long ns;
			char sign;

			case 'c':
				output_c(&os, va_arg(argp, int), 1);
				break;

			case 's':
				const char *s = va_arg(argp, char*);
				if (s == NULL) s = "(null)";
				// TODO can segfault even if precision caps the string
				len = strlen(s);
				if (len > m.precision && m.precision != 0)
					len = m.precision;
				pad(&os, &m, len);
				output(&os, s, len);
				break;

			case 'p':
				output(&os, "0x", 2);
				output_uint16(&os, &m, (uintptr_t)va_arg(argp, void*));
				break;

			case 'x':
			case 'u':
			case 'o':
				     if (lm == LM_int)      n = va_arg(argp, unsigned int);
				else if (lm == LM_long)     n = va_arg(argp, unsigned long);
				else if (lm == LM_longlong) n = va_arg(argp, unsigned long long);
				else if (lm == LM_size)     n = va_arg(argp, size_t);
				if (c == 'x') output_uint16(&os, &m, n);
				if (c == 'u') output_uint(&os, &m, n, '\0');
				if (c == 'o') output_octal(&os, &m, n);
				break;

			case 'd':
			case 'i':
				     if (lm == LM_int)      ns = va_arg(argp, int);
				else if (lm == LM_long)     ns = va_arg(argp, long);
				else if (lm == LM_longlong) ns = va_arg(argp, long long);
				else if (lm == LM_size)     ns = va_arg(argp, size_t);
				sign = '\0';
				if (ns < 0) {
					ns = -ns;
					sign = '-';
				}
				output_uint(&os, &m, (long)ns, sign);
				break;

			case '%':
				output(&os, "%", 1);
				break;
		}
		seg = fmt;
	}
}


static void vsnprintf_backend(void *arg, const char *buf, size_t len) {
	char **ptrs = arg;
	size_t space = ptrs[1] - ptrs[0];
	if (!ptrs[0]) return;
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
	return __printf_internal(fmt, ap, vsnprintf_backend, &ptrs);
}

int snprintf(char *restrict str, size_t len, const char *restrict fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = vsnprintf(str, len, fmt, argp);
	va_end(argp);
	return ret;
}

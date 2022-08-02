#include <stdbool.h>
#include <shared/mem.h>
#include <shared/printf.h>

struct out_state {
	void (*back)(void *, const char *, size_t);
	void *backarg;
	int written;
};

static int output(struct out_state *os, const char *buf, size_t len) {
	if (len) os->back(os->backarg, buf, len);
	os->written += len;
	return os->written;
}

static void output_c(struct out_state *os, char c) {
	output(os, &c, 1);
}


struct mods {
	char fill_char;
	size_t field_width;
};

static void pad(struct out_state *os, struct mods *m, size_t len) {
	for (size_t i = 0; len + i < m->field_width; i++)
		output(os, &m->fill_char, 1);
}

static void output_uint(struct out_state *os, struct mods *m, unsigned long long n) {
	// TODO static assert sizeof(unsigned long long) <= 8
	char buf[20];
	size_t pos = sizeof(buf);

	if (!n) {
		buf[--pos] = '0';
	} else while (n) {
		unsigned long long q = n / 10, r = n % 10;
		buf[--pos] = r + '0';
		n = q;
	}
	pad(os, m, sizeof(buf) - pos);
	output(os, buf + pos, sizeof(buf) - pos);
}


int __printf_internal(const char *fmt, va_list argp,
		void (*back)(void*, const char*, size_t), void *backarg)
{
	const char *seg = fmt; /* beginning of the current non-modifier streak */
	struct out_state os = {
		.back = back,
		.backarg = backarg,
	};

	for (;;) {
		char c = *fmt++;
		if (c == '\0') {
			return output(&os, seg, fmt - seg - 1);
		}
		if (c != '%') continue;
		output(&os, seg, fmt - seg - 1);

		struct mods m = {
			.fill_char = ' ',
			.field_width = 0,
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

		// TODO length modifiers

		switch (c) {
			unsigned long n, len;

			case 'c':
				output_c(&os, va_arg(argp, int));
				break;

			case 's':
				const char *s = va_arg(argp, char*);
				if (s) {
					len = strlen(s);
					pad(&os, &m, len);
					output(&os, s, len);
				} else {
					pad(&os, &m, 0);
				}
				break;

			case 'x':
				n = va_arg(argp, unsigned long);
				len = 1;
				while (n >> (len * 4) && (len * 4) < (sizeof(n) * 8))
					len++;
				pad(&os, &m, len);
				while (len-- > 0) {
					char h = '0' + ((n >> (len * 4)) & 0xf);
					if (h > '9') h += 'a' - '9' - 1;
					output_c(&os, h);
				}
				break;

			case 'u':
				n = va_arg(argp, unsigned long);
				output_uint(&os, &m, n);
				break;

			case '%':
				output(&os, "%", 1);
				break;
		}
		seg = fmt;
	}
}

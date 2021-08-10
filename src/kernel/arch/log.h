#pragma once
#include <stddef.h>

void tty_init();
void tty_write(const char *buf, size_t len);

inline void _tty_hex(const char *buf, size_t len) {
	char hex[2];
	for (size_t i = 0; i < len; i++) {
		hex[0] = (buf[i] & 0xF0) >> 4;
		hex[0] += '0';
		if (hex[0] > '9')
			hex[0] += 'a' - '9' - 1;

		hex[1] = buf[i] & 0xF;
		hex[1] += '0';
		if (hex[1] > '9')
			hex[1] += 'a' - '9' - 1;
		
		tty_write(hex, 2);
	}
}

// used for static strings
#define tty_const(str) tty_write(str, sizeof(str) - 1)

// very hacky, shouldn't be actually used - only for debugging
// prints backwards
#define _tty_var(var) _tty_hex((void*)&var, sizeof(var))

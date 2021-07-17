#pragma once
#include <stddef.h>

void log_write(const char *buf, size_t len);

inline void log_hex(const char *buf, size_t len) {
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
		
		log_write(hex, 2);
	}
}

// used for static strings
#define log_const(str) log_write(str, sizeof(str) - 1)

// very hacky, shouldn't be actually used - only for debugging
// prints backwards
#define log_var_dont_use(var) log_hex((void*)&var, sizeof(var))

#pragma once
#include <stddef.h>

#define __NUM2STR(x) #x
#define NUM2STR(x)  __NUM2STR(x)

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

// see https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
#define min(a,b) ({ \
		typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a < _b ? _a : _b; })
#define max(a,b) ({ \
		typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a > _b ? _a : _b; })

#pragma once
#include <stddef.h>
#include <stdint.h>

#define __NUM2STR(x) #x
#define NUM2STR(x)  __NUM2STR(x)

// see https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
#define min(a,b) ({ \
		typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a < _b ? _a : _b; })
#define max(a,b) ({ \
		typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a > _b ? _a : _b; })

/* casts an uint32_t to int32_t, capping it at INT32_MAX instead of overflowing */
static inline int32_t capped_cast32(uint32_t u) {
	return (int32_t)min(u, (uint32_t)INT32_MAX);
}

static inline int clamp(int lo, int i, int hi) {
	if (i < lo) return lo;
	if (i > hi) return hi;
	return i;
}

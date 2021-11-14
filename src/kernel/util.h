#pragma once
#include <stddef.h>

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

#pragma once
#include <bits/panic.h>

static inline size_t mbstowcs(wchar_t *dst, const char *src, size_t n) {
	(void)dst; (void)src; (void)n;
	__libc_panic("unimplemented");
}

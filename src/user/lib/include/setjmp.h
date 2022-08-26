#pragma once
#include <user/lib/panic.h>

typedef char sigjmp_buf;
static inline int sigsetjmp(sigjmp_buf env, int savemask) {
	(void)env; (void)savemask;
	return 0;
}

static inline void siglongjmp(sigjmp_buf env, int val) {
	(void)env; (void)val;
	__libc_panic("unimplemented");
}

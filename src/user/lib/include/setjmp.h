#pragma once
#include <user/lib/panic.h>

typedef char jmp_buf[1];
typedef char sigjmp_buf[1];

static inline int setjmp(jmp_buf env) {
	(void)env;
	return 0;
}

static inline int sigsetjmp(sigjmp_buf env, int savemask) {
	(void)env; (void)savemask;
	return 0;
}

static inline _Noreturn void longjmp(jmp_buf env, int val) {
	(void)env; (void)val;
	__libc_panic("unimplemented");
}

static inline _Noreturn void siglongjmp(sigjmp_buf env, int val) {
	(void)env; (void)val;
	__libc_panic("unimplemented");
}

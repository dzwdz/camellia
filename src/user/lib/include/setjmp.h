#pragma once
#include <user/lib/panic.h>

typedef uint64_t jmp_buf[8]; /* rbx, rsp, rbp, r12, r13, r14, r15, rip */
typedef char sigjmp_buf[1];

int setjmp(jmp_buf env);
_Noreturn void longjmp(jmp_buf env, int val);

static inline int sigsetjmp(sigjmp_buf env, int savemask) {
	(void)env; (void)savemask;
	return 0;
}

static inline _Noreturn void siglongjmp(sigjmp_buf env, int val) {
	(void)env; (void)val;
	__libc_panic("unimplemented");
}

#include "../tests.h"
#include <stdbool.h>
#include <setjmp.h>

static void test_setjmp(void) {
	jmp_buf env;
	volatile bool inner;
	int val;
	inner = false;
	if (!(val = setjmp(env))) {
		inner = true;
		longjmp(env, 1234);
		test(0);
	} else {
		test(val == 1234);
		test(inner);
	}
	inner = false;
	if (!(val = setjmp(env))) {
		inner = true;
		longjmp(env, 0);
		test(0);
	} else {
		test(val == 1);
		test(inner);
	}
}

void r_libc_setjmp(void) {
	run_test(test_setjmp);
}

#define TEST_MACROS
#include <user/lib/stdlib.h>
#include <user/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static void run_forked(void (*fn)()) {
	if (!fork()) {
		fn();
		_syscall_exit(0);
	} else {
		/* successful tests must return 0
		 * TODO add a better fail msg */
		if (_syscall_await() != 0) test_fail();
	}
}


static void stress_fork(void) {
	for (size_t i = 0; i < 2048; i++) {
		if (!fork()) _syscall_exit(0);
		_syscall_await();
	}
}

void stress_all(void) {
	run_forked(stress_fork);
}

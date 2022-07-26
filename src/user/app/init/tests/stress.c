#define TEST_MACROS
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <user/app/init/tests/main.h>
#include <user/lib/stdlib.h>

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

#include "tests.h"
#include <camellia/compat.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <unistd.h>

static void run_forked(void (*fn)()) {
	if (!fork()) {
		fn();
		exit(0);
	} else {
		/* successful tests must return 0
		 * TODO add a better fail msg */
		if (_sys_await() != 0) test_fail();
	}
}


static void stress_fork(void) {
	for (size_t i = 0; i < 2048; i++) {
		if (!fork()) exit(0);
		_sys_await();
	}
}

void stress_all(void) {
	run_forked(stress_fork);
}

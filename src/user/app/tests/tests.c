#include "tests.h"
#include <camellia/syscalls.h>
#include <unistd.h>

void run_test(void (*fn)()) {
	if (!fork()) {
		fn();
		exit(0);
	} else {
		/* successful tests must return 0 */
		if (_syscall_await() != 0) test_fail();
	}
}

int main(void) {
	r_k_fs();
	r_k_misc();
	r_k_miscsyscall();
	r_libc_esemaphore();
	r_libc_string();
	r_s_printf();
	r_s_ringbuf();
	return 0;
}

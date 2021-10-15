#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/syscalls.h>

#define argify(str) str, sizeof(str) - 1

static void run_forked(void (*fn)()) {
	if (!_syscall_fork()) {
		fn();
		_syscall_exit(0);
	} else _syscall_await();
}


void test_all(void) {
	run_forked(test_await);
}

void test_await(void) {
	int ret;

	// regular exit()s
	if (!_syscall_fork()) _syscall_exit(69);
	if (!_syscall_fork()) _syscall_exit(420);

	// faults
	if (!_syscall_fork()) { // invalid memory access
		asm volatile("movb $69, 0" ::: "memory");
		printf("this shouldn't happen");
		_syscall_exit(-1);
	}
	if (!_syscall_fork()) { // #GP
		asm volatile("hlt" ::: "memory");
		printf("this shouldn't happen");
		_syscall_exit(-1);
	}

	while ((ret = _syscall_await()) != ~0)
		printf("await returned: %x\n", ret);
	printf("await: no more children\n");
}

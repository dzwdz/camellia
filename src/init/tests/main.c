#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/syscalls.h>

#define argify(str) str, sizeof(str) - 1

#define test_fail() do { \
	printf("\033[31m" "TEST FAILED: %s:%xh\n" "\033[0m", __func__, __LINE__); \
	return; \
} while (0)

#define assert(cond) if (!(cond)) test_fail();

static void run_forked(void (*fn)()) {
	if (!_syscall_fork()) {
		fn();
		_syscall_exit(0);
	} else _syscall_await();
}



static void test_await(void) {
	/* creates 16 child processes, each returning a different value. then checks
	 * if await() returns every value exactly once */
	int ret;
	int counts[16] = {0};

	for (int i = 0; i < 16; i++)
		if (!_syscall_fork())
			_syscall_exit(i);

	while ((ret = _syscall_await()) != ~0) {
		assert(0 <= ret && ret < 16);
		counts[ret]++;
	}

	for (int i = 0; i < 16; i++)
		assert(counts[i] == 1);
}

static void test_faults(void) {
	/* tests what happens when child processes fault.
	 * expected behavior: parent processes still manages to finish, and it can
	 * reap all its children */
	int await_cnt = 0;

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

	while (_syscall_await() != ~0) await_cnt++;
	assert(await_cnt == 2);
}


void test_all(void) {
	run_forked(test_await);
	run_forked(test_faults);
}

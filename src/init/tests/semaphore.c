#define TEST_MACROS
#include <init/lib/esemaphore.h>
#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static void odd(struct evil_sem *sem1, struct evil_sem *sem2) {
	printf("1");
	esem_signal(sem1);

	esem_wait(sem2);
	printf("3");
	esem_signal(sem1);

	esem_wait(sem2);
	printf("5");
	esem_signal(sem1);
}

static void even(struct evil_sem *sem1, struct evil_sem *sem2) {
	esem_wait(sem1);
	printf("2");
	esem_signal(sem2);

	esem_wait(sem1);
	printf("4");
	esem_signal(sem2);

	esem_wait(sem1);
	printf("6");
	esem_signal(sem2);
}

void test_semaphore(void) {
	struct evil_sem *sem1, *sem2;

	// TODO pipe-based test

	sem1 = esem_new(0);
	sem2 = esem_new(0);
	assert(sem1 && sem2);
	if (!_syscall_fork(0, NULL)) {
		odd(sem1, sem2);
		_syscall_exit(69);
	} else {
		even(sem1, sem2);
		assert(_syscall_await() == 69);
	}
	esem_free(sem1);
	esem_free(sem2);

	printf("\n");

	sem1 = esem_new(0);
	sem2 = esem_new(0);
	assert(sem1 && sem2);
	if (!_syscall_fork(0, NULL)) {
		even(sem1, sem2);
		_syscall_exit(69);
	} else {
		odd(sem1, sem2);
		assert(_syscall_await() == 69);
		_syscall_await();
	}
	esem_free(sem1);
	esem_free(sem2);

	printf("\n");
}

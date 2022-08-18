#include "../tests.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <user/lib/esemaphore.h>
#include <user/lib/thread.h>

int global_n;

static void basic_thread(void *sem) {
	global_n = 10;
	esem_signal(sem);
}

static void test_basic_thread(void) {
	struct evil_sem *sem = esem_new(0);
	global_n = 0;
	thread_create(FORK_NOREAP, basic_thread, sem);
	esem_wait(sem);
	test(global_n == 10);
}

void r_k_threads(void) {
	run_test(test_basic_thread);
}

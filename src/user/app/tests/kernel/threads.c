#include "../tests.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <string.h>
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


handle_t global_h;
static void shared_handle(void *sem) {
	handle_t ends[2];
	test(_syscall_pipe(ends, 0) >= 0);
	global_h = ends[0];
	esem_signal(sem);
	_syscall_write(ends[1], "Hello!", 7, -1, 0);
}
static void test_shared_handle(void) {
	struct evil_sem *sem = esem_new(0);
	char buf[16];
	global_h = -1;
	thread_create(FORK_NOREAP, shared_handle, sem);
	esem_wait(sem);

	test(global_h >= 0);
	test(_syscall_read(global_h, buf, sizeof buf, 0) == 7);
	test(!strcmp("Hello!", buf));
}

void r_k_threads(void) {
	run_test(test_basic_thread);
	run_test(test_shared_handle);
}

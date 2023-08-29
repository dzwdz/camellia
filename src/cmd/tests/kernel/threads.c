#include "../tests.h"
#include <camellia/compat.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <string.h>
#include <esemaphore.h>
#include <thread.h>

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

hid_t global_h;
static void shared_handle(void *sem) {
	hid_t ends[2];
	test(_sys_pipe(ends, 0) >= 0);
	global_h = ends[0];
	esem_signal(sem);
	_sys_write(ends[1], "Hello!", 7, -1, 0);
}
static void test_shared_handle(void) {
	struct evil_sem *sem = esem_new(0);
	char buf[16];
	global_h = -1;
	thread_create(FORK_NOREAP, shared_handle, sem);
	esem_wait(sem);

	test(global_h >= 0);
	test(_sys_read(global_h, buf, sizeof buf, 0) == 7);
	test(!strcmp("Hello!", buf));
}

static void many_thread(void *arg) {
	*(uint64_t*)arg += 10;
}
static void test_many_threads(void) {
	uint64_t n = 0;
	for (int i = 0; i < 10; i++) thread_create(0, many_thread, &n);
	for (int i = 0; i < 10; i++) _sys_await();
	test(n == 100);
}

void r_k_threads(void) {
	run_test(test_basic_thread);
	run_test(test_shared_handle);
	run_test(test_many_threads);
}

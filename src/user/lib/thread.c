#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <user/lib/thread.h>

void thread_create(int flags, void (*fn)(void*), void *arg) {
	if (!_syscall_fork(flags | FORK_SHAREMEM, NULL)) {
		void *stack = malloc(4096);
		chstack(arg, fn, stack + 4096);
	}
}

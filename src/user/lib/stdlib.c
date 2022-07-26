#include <camellia/syscalls.h>
#include <unistd.h>

int errno = 0;

int fork(void) {
	return _syscall_fork(0, NULL);
}

int close(handle_t h) {
	return _syscall_close(h);
}

_Noreturn void exit(int c) {
	_syscall_exit(c);
}

#include <assert.h>
#include <camellia/syscalls.h>
#include <stdio.h>

_Noreturn void __badassert(const char *func, const char *file, int line) {
	fprintf(stderr, "assertion failure %s:%s:%u\n", file, func, line);
	_sys_exit(1);
}

#include <init/stdlib.h>
#include <shared/printf.h>
#include <shared/syscalls.h>

int __stdin  = -1;
int __stdout = -1;

static void backend_file(void *arg, const char *buf, size_t len) {
	_syscall_write(*(handle_t*)arg, buf, len, -1);
}

int printf(const char *fmt, ...) {
	int ret = 0;
	va_list argp;
	va_start(argp, fmt);
	if (__stdout >= 0)
		ret = __printf_internal(fmt, argp, backend_file, (void*)&__stdout);
	va_end(argp);
	return ret;
}

#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/tty/tty.h>
#include <kernel/arch/generic.h>
#include <shared/printf.h>

void tty_init(void) {
	serial_preinit();
	serial_write("<3 ", 3);
}

static void backend(void __attribute__((unused)) *arg, const char *buf, size_t len) {
	serial_write(buf, len);
}

int kprintf(const char *fmt, ...) {
	int ret;
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend, NULL);
	va_end(argp);
	return ret;
}

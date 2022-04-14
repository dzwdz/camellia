#include <kernel/arch/generic.h>
#include <kernel/arch/i386/driver/serial.h>
#include <kernel/arch/i386/tty/vga.h>
#include <shared/printf.h>

void tty_init(void) {
	vga_clear();
	serial_init();

	vga_write("\x03 ", 2); // cp437 heart
	serial_write("<3 ", 3);
}

static void backend(void *arg, const char *buf, size_t len) {
	vga_write(buf, len);
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

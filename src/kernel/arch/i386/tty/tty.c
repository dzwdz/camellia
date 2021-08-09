#include <kernel/arch/i386/tty/serial.h>
#include <kernel/arch/i386/tty/vga.h>
#include <kernel/arch/log.h>

void tty_init() {
	vga_clear();
	serial_init();

	// write hearts
	vga_write("\x03 ", 2);
	serial_write("<3 ", 3);
}

void tty_write(const char *buf, size_t len) {
	vga_write(buf, len);
	serial_write(buf, len);
}

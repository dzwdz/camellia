#include <kernel/arch/i386/tty/vga.h>
#include <kernel/arch/log.h>

void tty_init() {
	vga_clear();
}

void tty_write(const char *buf, size_t len) {
	vga_write(buf, len);
}

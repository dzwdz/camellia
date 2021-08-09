#include <kernel/arch/i386/tty/vga.h>
#include <kernel/arch/log.h>

void log_write(const char *buf, size_t len) {
	tty_write(buf, len);
}

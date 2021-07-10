#include <arch/i386/tty.h>
#include <arch/log.h>

void log_write(const char *buf, size_t len) {
	tty_write(buf, len);
}

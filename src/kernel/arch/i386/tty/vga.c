#include <kernel/arch/i386/tty/vga.h>

struct vga_cell {
	unsigned char c;
	unsigned char style;
} __attribute__((__packed__));

static const size_t     vga_len = 80 * 25;
static struct vga_cell *vga     = (void*) 0xB8000;
static size_t           vga_pos = 0;

static void tty_scroll() {
	for (size_t i = 0; i < vga_len - 80; i++)
		vga[i] = vga[i + 80];
	vga_pos -= 80;
}

void tty_putchar(char c) {
	if (vga_pos >= vga_len - 80)
		tty_scroll();
	vga[vga_pos++].c = c;
}

void tty_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		tty_putchar(buf[i]);
}

void tty_clear() {
	for (size_t i = 0; i < vga_len; i++)
		vga[i].c = ' ';
	vga_pos = 0;
}

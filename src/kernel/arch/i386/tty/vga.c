#include <kernel/arch/i386/tty/vga.h>

struct vga_cell {
	unsigned char c;
	unsigned char style;
} __attribute__((__packed__));

static const size_t     vga_len = 80 * 25;
static struct vga_cell *vga     = (void*) 0xB8000;
static size_t           vga_pos = 0;

static void vga_scroll() {
	for (size_t i = 0; i < vga_len - 80; i++)
		vga[i] = vga[i + 80];
	vga_pos -= 80;
}

void vga_putchar(char c) {
	if (vga_pos >= vga_len - 80)
		vga_scroll();
	vga[vga_pos++].c = c;
}

void vga_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		vga_putchar(buf[i]);
}

void vga_clear() {
	for (size_t i = 0; i < vga_len; i++)
		vga[i].c = ' ';
	vga_pos = 0;
}

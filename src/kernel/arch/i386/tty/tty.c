#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/tty/serial.h>
#include <kernel/arch/i386/tty/vga.h>
#include <kernel/arch/io.h>

void tty_init(void) {
	vga_clear();
	serial_init();

	// write hearts
	vga_write("\x03 ", 2);
	serial_write("<3 ", 3);
}

void tty_read(char *buf, size_t len) {
	irq_interrupt_flag(true);
	for (size_t i = 0; i < len; i++) {
		for (;;) {
			if (serial_poll_read(&buf[i]))	break;
			asm("hlt" ::: "memory");
		}
	}
	irq_interrupt_flag(false);
}

void tty_write(const char *buf, size_t len) {
	vga_write(buf, len);
	serial_write(buf, len);
}

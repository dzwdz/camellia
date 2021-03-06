#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <stdint.h>

static const int PIC1 = 0x20;
static const int PIC2 = 0xA0;

void irq_init(void) {
	port_out8(PIC1, 0x11);   /* start init sequence */
	port_out8(PIC2, 0x11);

	port_out8(PIC1+1, 0x20); /* interrupt offsets */
	port_out8(PIC2+1, 0x30);

	port_out8(PIC1+1, 0x4);  /* just look at the osdev wiki lol */
	port_out8(PIC2+1, 0x2);

	port_out8(PIC1+1, 0x1);  /* 8086 mode */
	port_out8(PIC2+1, 0x1);

	uint8_t mask = 0xff;
	mask &= ~(1 << 1); // keyboard
	mask &= ~(1 << 4); // COM1

	port_out8(PIC1+1, mask);
	port_out8(PIC2+1, 0xff);
}

void irq_eoi(uint8_t line) {
	port_out8(PIC1, 0x20);
	if (line >= 8)
		port_out8(PIC2, 0x20);
}

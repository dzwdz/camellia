#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <stdint.h>

static const int PIC1 = 0x20;
static const int PIC2 = 0xA0;

void irq_init(void) {
	port_out8(PIC1, 0x11); /* start init sequence */
	port_out8(PIC2, 0x11);

	port_out8(PIC1+1, IRQ_IBASE); /* interrupt offsets */
	port_out8(PIC2+1, IRQ_IBASE + 8);

	port_out8(PIC1+1, 0x4);
	port_out8(PIC2+1, 0x2);
	port_out8(PIC1+1, 0x1);
	port_out8(PIC2+1, 0x1);

	uint8_t mask = 0xff;
	mask &= ~(1 << IRQ_PS2);
	mask &= ~(1 << IRQ_COM1);

	port_out8(PIC1+1, mask);
	port_out8(PIC2+1, 0xff);
}

void irq_eoi(uint8_t line) {
	port_out8(PIC1, 0x20);
	if (line >= 8)
		port_out8(PIC2, 0x20);
}

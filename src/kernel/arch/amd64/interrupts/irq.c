#include <kernel/arch/amd64/interrupts.h>
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

	uint16_t mask = 0xffff;
	mask &= ~(1 << 2); // cascade
	mask &= ~(1 << IRQ_COM1);
	mask &= ~(1 << IRQ_PS2KB);
	mask &= ~(1 << IRQ_PS2MOUSE);
	mask &= ~(1 << IRQ_RTL8139);
	mask &= ~(1 << IRQ_HPET);

	port_out8(PIC1+1, mask & 0xff);
	port_out8(PIC2+1, (mask >> 8) & 0xff);
}

void irq_eoi(uint8_t line) {
	port_out8(PIC1, 0x20);
	if (line >= 8)
		port_out8(PIC2, 0x20);
}

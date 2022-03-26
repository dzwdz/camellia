#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/port_io.h>
#include <stdint.h>

static const int PIC1 = 0x20;
static const int PIC2 = 0xA0;

static void irq_unmask(uint8_t line) {
	uint16_t pic = line < 8 ? PIC1 : PIC2;
	line &= 7;

	port_out8(pic+1, port_in8(pic+1) & ~(1 << line));
}

void irq_init(void) {
	port_out8(PIC1, 0x11);  /* start init sequence */
	port_out8(PIC2, 0x11);

	port_out8(PIC1+1, 0x20);   /* interrupt offsets */
	port_out8(PIC2+1, 0x30);

	port_out8(PIC1+1, 0x4);   /* just look at the osdev wiki lol */
	port_out8(PIC2+1, 0x2);

	port_out8(PIC1+1, 0x1);   /* 8086 mode */
	port_out8(PIC2+1, 0x1);

	port_out8(PIC1+1, 0xfd); /* mask */
	port_out8(PIC2+1, 0xff);
}

void irq_eoi(uint8_t line) {
	port_out8(PIC1, 0x20);
	if (line >= 8)
		port_out8(PIC2, 0x20);
}

void irq_interrupt_flag(bool flag) {
	if (flag)	asm("sti" : : : "memory");
	else		asm("cli" : : : "memory");
}

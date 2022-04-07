#include <kernel/arch/i386/driver/serial.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/port_io.h>
#include <kernel/panic.h>
#include <stdint.h>

static const int COM1 = 0x3f8;

static void serial_selftest(void) {
	char b = 0x69;
	port_out8(COM1 + 4, 0b00011110); // enable loopback mode
	port_out8(COM1, b);
	assert(port_in8(COM1) == b);
}

void serial_init(void) {
	// see https://www.sci.muni.cz/docs/pc/serport.txt
	// set baud rate divisor
	port_out8(COM1 + 3, 0b10000000); // enable DLAB
	port_out8(COM1 + 0, 0x01);       // divisor = 1 (low  byte)
	port_out8(COM1 + 1, 0x00);       //             (high byte)

	port_out8(COM1 + 3, 0b00000011); // 8 bits, no parity, one stop bit
	port_out8(COM1 + 1, 0x01);       // enable the Data Ready IRQ
	port_out8(COM1 + 2, 0b11000111); // enable FIFO with 14-bit trigger level (???)

	serial_selftest();

	port_out8(COM1 + 4, 0b00001111); // enable everything in the MCR
}

static void serial_putchar(char c) {
	while ((port_in8(COM1 + 5) & 0x20) == 0); // wait for THRE
	port_out8(COM1, c);
}

bool serial_poll_read(char *c) {
	if ((port_in8(COM1 + 5) & 0x01) == 0) // needs DR
		return false;
	*c = port_in8(COM1);
	return true;
}

size_t serial_read(char *buf, size_t len) {
	irq_interrupt_flag(true);
	for (size_t i = 0; i < len; i++) {
		while (!serial_poll_read(&buf[i]))
			asm("hlt" ::: "memory");
	}
	irq_interrupt_flag(false);
	// TODO root driver assumes no partial reads
	return len;
}

void serial_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putchar(buf[i]);
}

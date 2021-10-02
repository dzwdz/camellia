#include <kernel/arch/i386/port_io.h>
#include <kernel/arch/i386/tty/serial.h>
#include <kernel/panic.h>
#include <stdint.h>

static const int COM1 = 0x3f8;

static void serial_selftest(void) {
	char b = 0x69;
	port_outb(COM1 + 4, 0b00011110); // enable loopback mode
	port_outb(COM1, b);
	assert(port_inb(COM1) == b);
}

void serial_init(void) {
	// see https://www.sci.muni.cz/docs/pc/serport.txt

	port_outb(COM1 + 1, 0x00); // disable interrupts, we won't be using them

	// set baud rate divisor
	port_outb(COM1 + 3, 0b10000000); // enable DLAB
	port_outb(COM1 + 0, 0x01);       // divisor = 1 (low  byte)
	port_outb(COM1 + 1, 0x00);       //             (high byte)

	port_outb(COM1 + 3, 0b00000011); // 8 bits, no parity, one stop bit
	port_outb(COM1 + 2, 0b11000111); // enable FIFO with 14-bit trigger level (???)

	serial_selftest();

	port_outb(COM1 + 4, 0b00001111); // enable everything in the MCR
}

static void serial_putchar(char c) {
	while ((port_inb(COM1 + 5) & 0x20) == 0); // wait for THRE
	port_outb(COM1, c);
}

char serial_read(void) {
	while ((port_inb(COM1 + 5) & 0x01) == 0); // wait for DR
	return port_inb(COM1);
}

void serial_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putchar(buf[i]);
}

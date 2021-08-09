#include <kernel/arch/i386/tty/serial.h>
#include <kernel/arch/i386/port_io.h>
#include <stdint.h>

const int COM1 = 0x3f8;

void serial_init() {
	// see https://www.sci.muni.cz/docs/pc/serport.txt

	port_outb(COM1 + 1, 0x00); // disable interrupts, we won't be using them

	// set baud rate divisor
	port_outb(COM1 + 3, 0b10000000); // enable DLAB
	port_outb(COM1 + 0, 0x01);       // divisor = 1 (low  byte)
	port_outb(COM1 + 1, 0x00);       //             (high byte)

	port_outb(COM1 + 3, 0b00000011); // 8 bits, no parity, one stop bit
	port_outb(COM1 + 2, 0b11000111); // enable FIFO with 14-bit trigger level (???)
	port_outb(COM1 + 4, 0b00001111); // enable everything in the MCR

	/* the example on the OSDEV wiki does a selftest of the serial connection
	 * i don't really see the point as long as i'm not using it for input?
	 * if i start using serial for input, TODO selftest */
}

void serial_putchar(char c) {
	while ((port_inb(COM1 + 5) & 0x20) == 0); // wait for THRE
	port_outb(COM1, c);
}

void serial_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putchar(buf[i]);
}

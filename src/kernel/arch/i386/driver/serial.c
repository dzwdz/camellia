#include <kernel/arch/i386/driver/serial.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/port_io.h>
#include <kernel/panic.h>
#include <shared/mem.h>
#include <stdint.h>


#define BACKLOG_CAPACITY 64
static volatile uint8_t backlog[BACKLOG_CAPACITY] = {};
static volatile size_t backlog_size = 0;

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


bool serial_ready(void) {
	return backlog_size > 0;
}

void serial_irq(void) {
	if (backlog_size >= BACKLOG_CAPACITY) return;
	backlog[backlog_size++] = port_in8(COM1);
}

size_t serial_read(char *buf, size_t len) {
	// copied from ps2, maybe could be made into a shared function? TODO FIFO lib
	if (backlog_size <= len)
		len = backlog_size;
	backlog_size -= len; /* guaranteed to never be < 0 */
	memcpy(buf, (void*)backlog, len);

	/* move rest of buffer back on partial reads */
	// TODO assumes that memcpy()ing into an overlapping buffer is fine, outside spec
	if (backlog_size > 0)
		memcpy((void*)backlog, (void*)backlog + len, backlog_size);

	return len;
}


static void serial_putchar(char c) {
	while ((port_in8(COM1 + 5) & 0x20) == 0); // wait for THRE
	port_out8(COM1, c);
}

void serial_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putchar(buf[i]);
}

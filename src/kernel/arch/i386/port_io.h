#include <stdint.h>

static inline void port_out8(uint16_t port, uint8_t val) {
	asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
}

static inline uint8_t port_in8(uint16_t port) {
	uint8_t val;
	asm volatile("inb %1, %0" : "=a" (val) : "Nd" (port));
	return val;
}


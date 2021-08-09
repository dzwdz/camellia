#include <stdint.h>

inline void port_outb(uint16_t port, uint8_t val) {
	asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
}

inline uint8_t port_inb(uint16_t port) {
	uint8_t val;
	asm volatile("inb %1, %0" : "=a" (val) : "Nd" (port));
	return val;
}


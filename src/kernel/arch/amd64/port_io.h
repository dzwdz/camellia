#include <stdint.h>

static inline void port_out8(uint16_t port, uint8_t val) {
	asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
}

static inline void port_out16(uint16_t port, uint16_t val) {
	asm volatile("outw %0, %1" : : "a" (val), "Nd" (port));
}

static inline void port_out32(uint16_t port, uint32_t val) {
	asm volatile("outl %0, %1" : : "a" (val), "Nd" (port));
}

static inline uint8_t port_in8(uint16_t port) {
	uint8_t val;
	asm volatile("inb %1, %0" : "=a" (val) : "Nd" (port));
	return val;
}

static inline uint16_t port_in16(uint16_t port) {
	uint16_t val;
	asm volatile("inw %1, %0" : "=a" (val) : "Nd" (port));
	return val;
}

static inline uint32_t port_in32(uint16_t port) {
	uint32_t val;
	asm volatile("inl %1, %0" : "=a" (val) : "Nd" (port));
	return val;
}


#include <kernel/arch/amd64/32/gdt.h>
#include <kernel/arch/amd64/interrupts/idt.h>
#include <kernel/arch/amd64/interrupts/isr.h>
#include <kernel/panic.h>
#include <stdbool.h>
#include <stdint.h>

struct idt_entry {
	uint16_t offset_low;
	uint16_t seg;
	uint8_t ist;
	uint8_t type    : 4; // 0xE - interrupt, 0xF - trap
	uint8_t zero1   : 1;
	uint8_t ring    : 2;
	uint8_t present : 1;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t zero2;
} __attribute__((packed));

// is exactly the same as lgdt_arg, i should combine them into a single struct
// later
struct lidt_arg {
	uint16_t limit;
	uintptr_t base;
} __attribute__((packed));

__attribute__((section(".shared")))
static struct idt_entry IDT[256];
static struct lidt_arg lidt_arg;

static void idt_prepare(void);
static void idt_load(void);
static void idt_test(void);


static void idt_prepare(void) {
	for (int i = 0; i < 256; i++) {
		uintptr_t offset = (uintptr_t) &_isr_stubs + i * 8;

		IDT[i] = (struct idt_entry) {
			.offset_low  = offset,
			.offset_mid  = offset >> 16,
			.offset_high = offset >> 32,
			.seg = SEG_r0code << 3,
			.present = 1,
			.type = 0xE,
			.ist = 1,
		};
	}
}

static void idt_load(void) {
	lidt_arg.limit = sizeof(IDT) - 1;
	lidt_arg.base = (uintptr_t) &IDT;
	asm("lidt (%0)" : : "r" (&lidt_arg) : "memory");
}

static void idt_test(void) {
	asm("int $0x34" : : : "memory");
	assert(isr_test_interrupt_called);
}

void idt_init(void) {
	idt_prepare();
	idt_load();
	idt_test();
}

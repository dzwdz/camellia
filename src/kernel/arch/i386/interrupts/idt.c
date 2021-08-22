#include <kernel/arch/i386/gdt.h>
#include <kernel/arch/i386/interrupts/idt.h>
#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/panic.h>
#include <stdbool.h>
#include <stdint.h>

struct idt_entry {
	uint16_t offset_low   ;
	uint16_t code_seg     ;
	uint8_t  zero         ; // unused, has to be 0
	uint8_t  type      : 4; // 16/32 bit, task/interrupt/task gate
	uint8_t  storage   : 1; // 0 for interrupt/trap gates
	uint8_t  ring      : 2;
	uint8_t  present   : 1;
	uint16_t offset_high  ;
} __attribute__((packed));

// is exactly the same as lgdt_arg, i should combine them into a single struct
// later
struct lidt_arg {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));


static struct idt_entry IDT[256];
static struct lidt_arg lidt_arg;

static void idt_prepare();
static void idt_load();
static void idt_test();


static void idt_prepare() {
	uintptr_t offset;
	for (int i = 0; i < 256; i++) {
		offset = (uintptr_t) &_isr_stubs + i * 8;

		IDT[i] = (struct idt_entry) {
			.offset_low  = offset,
			.offset_high = offset >> 16,
			.code_seg    = SEG_r0code << 3,
			.zero        = 0,
			.present     = 1,
			.ring        = 0,
			.storage     = 0,
			.type        = 0xE, // 32-bit interrupt gate
		};
	}
}

static void idt_load() {
	lidt_arg.limit = sizeof(IDT) - 1;
	lidt_arg.base = (uintptr_t) &IDT;
	asm("lidt (%0)" : : "r" (&lidt_arg) : "memory");
}

static void idt_test() {
	asm("int $0x34" : : : "memory");
	assert(isr_test_interrupt_called);
}

void idt_init() {
	idt_prepare();
	idt_load();
	idt_test();
}

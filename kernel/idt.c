#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/tty.h> // used only for the selftest
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

static inline void idt_add(uint8_t num, bool user, void (*isr));
static void idt_prepare();
static void idt_load();
static void idt_test();


static inline void idt_add(uint8_t num, bool user, void (*isr)) {
	uintptr_t offset = (uintptr_t) isr;

	IDT[num] = (struct idt_entry) {
		.offset_low  = offset,
		.offset_high = offset >> 16,
		.code_seg    = SEG_r0code << 3,
		.zero        = 0,
		.present     = 1,
		.ring        = user ? 3 : 0,
		.storage     = 0,
		.type        = 0xE, // 32-bit interrupt gate
	};
}

static void idt_prepare() {
	for (int i = 0; i < 256; i++)
		IDT[i].present = 0;

	idt_add(0x08, false, isr_double_fault);
	idt_add(0x34, false, isr_test_interrupt);
}

static void idt_load() {
	lidt_arg.limit = sizeof(IDT) - 1;
	lidt_arg.base = (uintptr_t) &IDT;
	asm("lidt (%0)" : : "r" (&lidt_arg) : "memory");
}

static void idt_test() {
	asm("int $0x34" : : : "memory");
	if (!isr_test_interrupt_called) {
		tty_const("IDT self test failed");
		for(;;);
	}
}

void idt_init() {
	idt_prepare();
	idt_load();
	idt_test();
}

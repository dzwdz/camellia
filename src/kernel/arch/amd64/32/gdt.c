#include <kernel/arch/amd64/32/gdt.h>
#include <kernel/arch/generic.h>
#include <stdbool.h>
#include <stdint.h>

extern char _isr_mini_stack;

struct gdt_entry {
	uint64_t limit_low  : 16;
	uint64_t base_low   : 24;
	uint64_t accessed   :  1; // set by the processor
	                          // CODE        | DATA
	uint64_t rw         :  1; // readable?   | writeable?
	uint64_t conforming :  1; // conforming? | expands down?
	uint64_t code       :  1; // 1           | 0

	uint64_t codeordata :  1; // 1 for everything other than TSS and LDT
	uint64_t ring       :  2;
	uint64_t present    :  1; // always 1
	uint64_t limit_high :  4;
	uint64_t available  :  1; // ???
	uint64_t long_mode  :  1;
	uint64_t x32        :  1;
	uint64_t gran       :  1; // 1 - 4kb, 0 - 1b
	uint64_t base_high  :  8;
} __attribute__((packed));

struct tss_entry {
	uint32_t reserved0;
	uint64_t rsp[3];
	uint64_t ist[8];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb;
} __attribute__((packed));

struct lgdt_arg {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

__attribute__((section(".shared")))
struct gdt_entry GDT[SEG_end];
__attribute__((section(".shared")))
struct tss_entry TSS;

struct lgdt_arg lgdt_arg;


static void *memset32(void *s, int c, size_t n) {
	uint8_t *s2 = s;
	for (size_t i = 0; i < n; i++)
		s2[i] = c;
	return s;
}


static void gdt_fillout(struct gdt_entry* entry, uint8_t ring, bool code) {
	*entry = (struct gdt_entry) {
		// set up the identity mapping
		.limit_low  = 0xFFFF,
		.limit_high = 0xF,
		.gran       = 1,
		.base_low   = 0,
		.base_high  = 0,

		.ring       = ring,
		.code       = code,

		.accessed   = 0,
		.rw         = 1,
		.conforming = 0,
		.codeordata = 1,
		.present    = 1,
		.long_mode  = 1,
		.available  = 1,
		.x32        = 0,
	};
}

void gdt_init(void) {
	GDT[SEG_null].present = 0;

	gdt_fillout(&GDT[SEG_r0code], 0, true);
	gdt_fillout(&GDT[SEG_r0data], 0, false);
	gdt_fillout(&GDT[SEG_r3code], 3, true);
	gdt_fillout(&GDT[SEG_r3data], 3, false);

	lgdt_arg.limit = sizeof(GDT) - 1;
	lgdt_arg.base = (uint64_t)(int)&GDT;


	memset32(&TSS, 0, sizeof(TSS));
	for (int i = 0; i < 3; i++)
		TSS.rsp[i] = (uint64_t)(int)&_isr_mini_stack;
	TSS.ist[1] = (uint64_t)(int)&_isr_mini_stack;

	uint64_t tss_addr = (uint64_t)(int)&TSS;
	GDT[SEG_TSS] = (struct gdt_entry) {
		.limit_low  = sizeof(TSS),
		.limit_high = sizeof(TSS) >> 16,
		.gran       = 0,
		.base_low   = tss_addr,
		.base_high  = tss_addr >> 24,

		.accessed   = 1,
		.rw         = 0,
		.conforming = 0,
		.code       = 1,
		.codeordata = 0,
		.ring       = 0, // was 3 pre-port
		.present    = 1,
		.available  = 1,
		.long_mode  = 0,
		.x32        = 0,
	};
	*((uint64_t*)&GDT[SEG_TSS2]) = (tss_addr >> 32);
}

#include <kernel/arch/generic.h>
#include <kernel/arch/i386/gdt.h>
#include <kernel/util.h>
#include <stdbool.h>
#include <stdint.h>


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
	uint32_t _unused0;
	uint32_t esp0; // kernel mode stack pointer
	uint32_t ss0;  // kernel mode stack segment
	uint8_t  _unused1[0x5c];
} __attribute__((packed));

struct lgdt_arg {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

static struct gdt_entry GDT[SEG_end];
static struct tss_entry TSS;
static struct lgdt_arg lgdt_arg; // probably doesn't need to be global

static void gdt_fillout(struct gdt_entry* entry, uint8_t ring, bool code);
static void gdt_prepare(void);
static void gdt_load(void);


static void gdt_fillout(struct gdt_entry* entry, uint8_t ring, bool code) {
	*entry = (struct gdt_entry) {
		// set up the identity mapping
		.limit_low  = 0xFFFF,
		.limit_high = 0xF,
		.gran       = 1, // 4KB * 0xFFFFF = (almost) 4GB
		.base_low   = 0,
		.base_high  = 0,

		.ring       = ring,
		.code       = code,

		.accessed   = 0,
		.rw         = 1,
		.conforming = 0,
		.codeordata = 1,
		.present    = 1,
		.long_mode  = 0, // ???
		.available  = 1, // ???
		.x32        = 1,
	};
}

static void gdt_prepare(void) {
	GDT[SEG_null].present = 0;

	gdt_fillout(&GDT[SEG_r0code], 0, true);
	gdt_fillout(&GDT[SEG_r0data], 0, false);
	gdt_fillout(&GDT[SEG_r3code], 3, true);
	gdt_fillout(&GDT[SEG_r3data], 3, false);

	// tss
	memset(&TSS, 0, sizeof(TSS));
	TSS.ss0 = SEG_r0data << 3; // kernel data segment
	TSS.esp0 = (uintptr_t) &_bss_end;

	GDT[SEG_TSS] = (struct gdt_entry) {
		.limit_low  = sizeof(TSS),
		.limit_high = sizeof(TSS) >> 16,
		.gran       = 0,
		.base_low   =  (uintptr_t) &TSS,
		.base_high  = ((uintptr_t) &TSS) >> 24,

		.accessed   = 1, // 1 for TSS
		.rw         = 0, // 1 busy / 0 not busy
		.conforming = 0, // 0 for TSS
		.code       = 1, // 32bit
		.codeordata = 0, // is a system entry
		.ring       = 3,
		.present    = 1,
		.available  = 0, // 0 for TSS
		.long_mode  = 0,
		.x32        = 0, // idk
	};
}

static void gdt_load(void) {
	lgdt_arg.limit = sizeof(GDT) - 1;
	lgdt_arg.base = (uintptr_t) &GDT;
	asm("lgdt (%0)" 
	    : : "r" (&lgdt_arg) : "memory");

	asm("ltr %%ax"
	    : : "a" (SEG_TSS << 3 | 3) : "memory");

	// update all segment registers
	gdt_farjump(SEG_r0code << 3);
	asm("mov %0, %%ds;"
	    "mov %0, %%ss;"
	    "mov %0, %%es;"
	    "mov %0, %%fs;"
	    "mov %0, %%gs;"
	    : : "r" (SEG_r0data << 3) : "memory");
}

void gdt_init(void) {
	gdt_prepare();
	gdt_load();
}

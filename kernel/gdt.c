#include <kernel/tty.h>
#include <stdint.h>

extern void stack_top; // platform/boot.s

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
	uint32_t prev_tss; // unused
	uint32_t esp0; // kernel mode stack pointer
	uint32_t ss0;  // kernel mode stack segment
	// total size = 0x68 (?) - 3 * sizeof(uint32_t) = 5c
	uint8_t  _unused[0x5c];
} __attribute__((packed));

struct lgdt_arg {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

enum {
	SEG_null,
	SEG_r0data,
	SEG_r0code,
	SEG_r3data,
	SEG_r3code,
	SEG_TSS,

	SEG_end    
};
static struct gdt_entry GDT[SEG_end];
static struct tss_entry TSS;
static struct lgdt_arg lgdt_arg; // probably doesn't need to be global

static void gdt_prepare();
static void gdt_load();
static void gdt_check();


static void gdt_prepare() {
	GDT[SEG_null].present = 0;

	GDT[SEG_r0data].limit_low  = 0xFFFF;
	GDT[SEG_r0data].limit_high = 0xF;
	GDT[SEG_r0data].gran       = 1; // 4KB * 0xFFFFF = (almost) 4GB
	GDT[SEG_r0data].base_low   = 0;
	GDT[SEG_r0data].base_high  = 0;
	GDT[SEG_r0data].accessed   = 0;
	GDT[SEG_r0data].rw         = 1;
	GDT[SEG_r0data].conforming = 0;
	GDT[SEG_r0data].code       = 0;
	GDT[SEG_r0data].codeordata = 1;
	GDT[SEG_r0data].ring       = 0;
	GDT[SEG_r0data].present    = 1;
	GDT[SEG_r0data].long_mode  = 0; // ???
	GDT[SEG_r0data].available  = 1; // ???
	GDT[SEG_r0data].x32        = 1;

	// copy to r0 code
	GDT[SEG_r0code] = GDT[SEG_r0data];
	GDT[SEG_r0code].code = SEG_r0data;

	GDT[SEG_r3data] = GDT[SEG_r0data];
	GDT[SEG_r3data].ring = 3;
	GDT[SEG_r3code] = GDT[SEG_r0code];
	GDT[SEG_r3data].ring = 3;

	// tss
	// TODO memset(&TSS, 0, sizeof(TSS));
	TSS.ss0 = SEG_r0data << SEG_r3data; // kernel data segment
	TSS.esp0 = (uint32_t) &stack_top;

	GDT[SEG_TSS].limit_low  = sizeof(TSS);
	GDT[SEG_TSS].base_low   = (uint32_t) &TSS;
	GDT[SEG_TSS].accessed   = 1; // 1 for TSS
	GDT[SEG_TSS].rw         = 0; // 1 busy / 0 not busy
	GDT[SEG_TSS].conforming = 0; // 0 for TSS
	GDT[SEG_TSS].code       = 1; // 32bit
	GDT[SEG_TSS].codeordata = 0; // is a system entry
	GDT[SEG_TSS].ring       = 3;
	GDT[SEG_TSS].present    = 1;
	GDT[SEG_TSS].limit_high = (sizeof(TSS) >> 16) & 0xf;
	GDT[SEG_TSS].available  = 0; // 0 for TSS
	GDT[SEG_TSS].long_mode  = 0;
	GDT[SEG_TSS].x32        = 0; // idk
	GDT[SEG_TSS].gran       = 0;
	GDT[SEG_TSS].base_high  = (((uint32_t) &TSS) >> 24) & 0xff;
}

static void gdt_load() {
	lgdt_arg.limit = sizeof(GDT) - 1;
	lgdt_arg.base = (uint32_t) &GDT;
	asm("lgdt (%0)" : : "b" (&lgdt_arg));
}

static void gdt_check() {
	tty_write("checking gdt...", 15);
	asm("mov %0, %%ds;"
	    : : "r" (SEG_r0data << 3));
	tty_write("ok", 2);
}

void gdt_init() {
	gdt_prepare();
	gdt_load();
	gdt_check();
}

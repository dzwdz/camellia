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
static struct gdt_entry GDT[6];
static struct tss_entry TSS;
static struct lgdt_arg lgdt_arg; // probably doesn't need to be global

static void gdt_prepare();
static void gdt_load();


static void gdt_prepare() {
	// null segment
	GDT[0].present = 0;

	// ring0 data
	GDT[1].limit_low  = 0xFFFF;
	GDT[1].limit_high = 0xF;
	GDT[1].gran       = 1; // 4KB * 0xFFFFF = (almost) 4GB

	GDT[1].base_low   = 0;
	GDT[1].base_high  = 0;
	GDT[1].accessed   = 0;
	GDT[1].rw         = 1;
	GDT[1].conforming = 0;
	GDT[1].code       = 0;
	GDT[1].codeordata = 1;
	GDT[1].ring       = 0;
	GDT[1].present    = 1;
	GDT[1].long_mode  = 0; // ???
	GDT[1].available  = 1; // ???
	GDT[1].x32        = 1;

	// copy to r0 code
	GDT[2] = GDT[1];
	GDT[2].code = 1;

	// r3 data & code
	GDT[3] = GDT[1];
	GDT[3].ring = 3;
	GDT[4] = GDT[2];
	GDT[3].ring = 3;

	{ // tss
		// TODO memset(&TSS, 0, sizeof(TSS));
		TSS.ss0 = 1 << 3; // kernel data segment
		TSS.esp0 = (uint32_t) &stack_top;

		GDT[5].limit_low  = sizeof(TSS);
		GDT[5].base_low   = (uint32_t) &TSS;
		GDT[5].accessed   = 1; // 0 for TSS
		GDT[5].rw         = 0; // 1 busy / 0 not busy
		GDT[5].conforming = 0; // 0 for TSS
		GDT[5].code       = 1; // 32bit
		GDT[5].codeordata = 0; // is a system entry
		GDT[5].ring       = 3;
		GDT[5].present    = 1;
		GDT[5].limit_high = (sizeof(TSS) >> 16) & 0xf;
		GDT[5].available  = 0; // 0 for TSS
		GDT[5].long_mode  = 0;
		GDT[5].x32        = 0; // idk
		GDT[5].gran       = 0;
		GDT[5].base_high  = (((uint32_t) &TSS) >> 24) & 0xff;
	}
}

static void gdt_load() {
	lgdt_arg.limit = sizeof(GDT) - 1;
	lgdt_arg.base = (uint32_t) &GDT;
	asm("lgdt (%0)" : : "b" (&lgdt_arg));
}

void gdt_init() {
	gdt_prepare();
	gdt_load();
	// check if the GDT was set up correctly
	tty_write("checking gdt...", 15);
	asm("mov $8, %%eax;"
	    "mov %%eax, %%ds;"
	    : : : "%eax");
	tty_write("ok", 2);
}

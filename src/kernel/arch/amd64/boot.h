#pragma once

enum gdt_segs {
	SEG_null,
	/* order dictated by SYSENTER */
	SEG_r0code = 1,
	SEG_r0data = 2,
	SEG_r3code32 = 3,
	SEG_r3data = 4,
	SEG_r3code = 5,
	SEG_TSS = 6,
	SEG_TSS2 = 7,

	SEG_end
};

void kmain_early(void *mbi);
void gdt_init(void);
void idt_init(void);

/* used from asm */
extern struct lgdt_arg lgdt_arg;

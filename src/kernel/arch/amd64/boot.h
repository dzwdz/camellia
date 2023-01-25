#pragma once

enum gdt_segs {
	SEG_null,
	/* order dictated by SYSENTER */
	SEG_r0code,
	SEG_r0data,
	SEG_r3code,
	SEG_r3data,
	SEG_TSS,
	SEG_TSS2,

	SEG_end
};

void kmain_early(void *mbi);
void gdt_init(void);
void idt_init(void);

/* used from asm */
extern struct lgdt_arg lgdt_arg;

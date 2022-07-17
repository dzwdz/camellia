#pragma once

enum {
	SEG_null,
	// order dictated by SYSENTER
	SEG_r0code,
	SEG_r0data,
	SEG_r3code,
	SEG_r3data,
	SEG_TSS,
	SEG_TSS2,

	SEG_end
};

void gdt_init(void);

extern struct lgdt_arg lgdt_arg; // used by amd64/32/boot.s

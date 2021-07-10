#pragma once

enum {
	SEG_null,
	// order dictated by SYSENTER
	SEG_r0code,
	SEG_r0data,
	SEG_r3code,
	SEG_r3data,
	SEG_TSS,

	SEG_end
};

void gdt_init();
void gdt_farjump(int segment);

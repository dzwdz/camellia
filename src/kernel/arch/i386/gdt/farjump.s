.section .text
.global gdt_farjump
.type gdt_farjump, @function
gdt_farjump:
	/* retf pops off the return address and code segment off the stack.
	 * it turns out that in the i386 cdecl calling convention they're in
	 * the correct place already.
	 */
	retf

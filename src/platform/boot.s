.set MAGIC, 0x1BADB002
.set FLAG_ALIGN,   1<<0 /* align modules on page boundaries */
.set FLAG_MEMINFO, 1<<1 /* memory map */
.set FLAGS,        FLAG_ALIGN | FLAG_MEMINFO
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* a lil stack */
.section .bss
.global stack_top
.type stack_top, @object
.align 16
stack_bottom:
.skip 16384
stack_top:


.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
	call kmain

.global halt_cpu
.type halt_cpu, @function
halt_cpu:
	cli
1:	hlt
	jmp 1b

// temporary, will be moved to another file soon
.global change_cs
.type change_cs, @function
change_cs:
	/* retf pops off the return address and code segment off the stack.
	 * it turns out that in the i386 cdecl calling convention they're in
	 * the correct place already.
	 */
	retf

.size _start, . - _start

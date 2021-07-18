.section .text
.global _start
.type _start, @function
_start:
	mov $_bss_end, %esp // the stack is at the top of bss
	push %ebx // address of the Multiboot struct
	call kmain_early

.global halt_cpu
.type halt_cpu, @function
halt_cpu:
	cli
1:	hlt
	jmp 1b

.size _start, . - _start

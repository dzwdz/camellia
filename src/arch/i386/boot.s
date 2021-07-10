/* a lil stack TODO move to linker.ld */
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
	call kmain_early

.global halt_cpu
.type halt_cpu, @function
halt_cpu:
	cli
1:	hlt
	jmp 1b

.size _start, . - _start

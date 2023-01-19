.section .text
.global intr_trampoline
.type intr_trampoline, @function
intr_trampoline:
	push %rax
	push %rdx
	call *_intr(%rip)
	pop %rdx
	pop %rax
	pop tmprip(%rip)
	pop %rsp
	jmp *tmprip(%rip)

.section .bss
tmprip:
	.skip 8

.global _intr
_intr:
	.skip 8

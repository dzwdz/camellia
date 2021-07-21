.section .text

.global _isr_stubs
_isr_stubs:

.rept 256
	.align 8
	pushal
	call _isr_stage2
.endr

_isr_stage2:
	cld

	// convert the return address into the vector nr
	pop %eax
	add $-_isr_stubs, %eax
	shr $3, %eax
	push %eax

	call isr_stage3
	add $4, %esp

	popal
	iret

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

		// disable paging, if present
		// it's done here so the stuff on the stack is in the right order
		mov %cr0, %ebx
		push %ebx
		and $0x7FFFFFFF, %ebx
		mov %ebx, %cr0

	push %eax       // push the vector nr
	call isr_stage3
	add $4, %esp    // "pop" the vector nr
	pop %eax        // restore old cr0
	mov %eax, %cr0

	popal
	iret

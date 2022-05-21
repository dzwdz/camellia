.section .text.early

.global _isr_stubs
_isr_stubs:
.rept 256
	pushal
	call _isr_stage2
	.align 8
.endr

_isr_stage2:
	cli

	// convert the return address into the vector nr
	pop %eax
	add $-_isr_stubs, %eax
	shr $3, %eax

	// disable paging, if present
	mov %cr0, %ebx
	push %ebx // push original cr0
	and $0x7FFFFFFF, %ebx
	mov %ebx, %cr0

	mov %esp, %ebp
	mov $_bss_end, %esp // switch to kernel stack
	push %eax // push the vector nr
	call isr_stage3

	mov %ebp, %esp // switch back to isr_stack
	pop %eax // restore old cr0
	mov %eax, %cr0

	popal
	iret

.align 8
_ist_stack_btm:
// TODO overflow check
.skip 64 // seems to be enough
.global _isr_stack_top
_isr_stack_top:

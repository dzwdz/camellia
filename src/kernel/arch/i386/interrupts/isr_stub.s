.section .shared

.global _isr_stubs
_isr_stubs:
.rept 256
	.set _stub_start, .

	cli
	call _isr_stage2

	.if . - _stub_start > 8
		.error "isr stubs over maximum size"
		.abort
	.endif
	.align 8
.endr

_isr_stage2:
	// pushal order, without %esp
	push %eax
	push %ecx
	push %edx
	push %ebx
	push %ebp
	push %esi
	push %edi

	// convert the return address into the vector nr
	mov 28(%esp), %eax
	add $-_isr_stubs, %eax
	shr $3, %eax

	// disable paging, if present
	mov %cr0, %ebx
	push %ebx // push original cr0
	and $0x7FFFFFFF, %ebx
	mov %ebx, %cr0

	mov %esp, %ebp
	mov $_isr_big_stack, %esp
	push %eax // push the vector nr
	call isr_stage3

	mov %ebp, %esp
	pop %eax // restore old cr0
	mov %eax, %cr0

	// restore registers
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax

	add $4, %esp // skip call's return address
	iret

.align 8
// TODO overflow check
.skip 64 // seems to be enough
.global _isr_mini_stack
_isr_mini_stack:

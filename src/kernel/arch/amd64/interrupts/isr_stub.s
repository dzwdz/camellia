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
	push %rax
	push %rcx
	push %rdx
	push %rbx
	push %rbp
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15

	// convert the return address into the vector nr
	mov 120(%rsp), %rdi
	sub $_isr_stubs, %rdi
	shr $3, %rdi

	lea 128(%rsp), %rsi // second argument - IRET stack frame

	// load kernel paging
	mov %cr3, %rbx
	push %rbx
	mov $pml4_identity, %rbx
	mov %rbx, %cr3

	mov %rsp, %rbp
	mov $_isr_big_stack, %rsp
	call isr_stage3

	mov %rbp, %rsp
	pop %rax // restore old cr3
	mov %rax, %cr3

	// restore registers
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rbp
	pop %rbx
	pop %rdx
	pop %rcx
	pop %rax

	add $8, %rsp // skip call's return address
	iretq

.align 8
// TODO stack overflow check
.skip 256 // seems to be enough
.global _isr_mini_stack
_isr_mini_stack:

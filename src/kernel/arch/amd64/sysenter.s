// gdt.h
.set SEG_r0code, 1
.set SEG_r3code, 3
.set SEG_r3data, 4

.set IA32_STAR, 0xC0000081
.set IA32_LSTAR, 0xC0000082
.set IA32_CSTAR, 0xC0000083
.set IA32_FMASK, 0xC0000084

.section .text
.global sysenter_setup
.type sysenter_setup, @function
sysenter_setup:

	// the intel docs don't mention the lower 32 bits
	mov $0, %eax
	mov $( ((SEG_r3code << 3 | 3) << 16) | (SEG_r0code << 3) ), %edx
	mov $IA32_STAR, %rcx
	wrmsr

	mov $sysenter_stage1, %eax
	mov $0, %edx
	mov $IA32_LSTAR, %rcx
	wrmsr

	// hoping that the default RFLAGS mask is fine...

	ret


.section .shared

.global stored_rsp
stored_rsp:
	.skip 8

.global pagedir_current
// a hack to maintain compat with the old arch api, TODO
pagedir_current:
	.skip 8

.global _sysexit_real
.type _sysexit_real, @function
_sysexit_real:
	/* TODO check if SYSRET correctly sets segments
	mov $(SEG_r3data << 3 | 3), %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	*/

	mov $_sysexit_regs, %rsp
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
	pop (stored_rsp)

	pop %rbx
	pop %rdx
	pop %rcx
	pop %rax

	// enable paging
	mov (pagedir_current), %rsp
	mov %rsp, %cr3
	mov (stored_rsp), %rsp
	sysretq

sysenter_stage1:
	cli

	mov %rsp, (stored_rsp)

	mov $pml4_identity, %rsp
	mov %rsp, %cr3

	mov $(_sysexit_regs + 128), %rsp
	push %rax
	push %rcx
	push %rdx
	push %rbx

	push (stored_rsp)
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

	mov $_bss_end, %rsp
	push $0 // for stacktraces
	jmp sysenter_stage2

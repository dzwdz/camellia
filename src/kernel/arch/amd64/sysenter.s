/* TODO include gdt.h */
.set SEG_r0code, 1
.set SEG_r3code, 3
.set SEG_r3data, 4

.set IA32_SYSENTER_CS, 0x174
.set IA32_SYSENTER_ESP, 0x175
.set IA32_SYSENTER_EIP, 0x176

.section .text
.global sysenter_setup
.type sysenter_setup, @function
sysenter_setup:
	xor %rdx, %rdx

	mov $(SEG_r0code << 3), %rax
	mov $IA32_SYSENTER_CS, %rcx
	wrmsr

	mov $IA32_SYSENTER_ESP, %rcx
	mov $0, %rax // unused
	wrmsr

	mov $IA32_SYSENTER_EIP, %rcx
	mov $sysenter_stage1, %rax
	wrmsr

	ret


.section .shared

.global stored_eax
stored_eax:
.long 0

.global pagedir_current
// a hack to maintain compat with the old arch api, TODO
pagedir_current:
.long 0

.global _sysexit_real
.type _sysexit_real, @function
_sysexit_real:
	xchgw %bx, %bx
	mov $(SEG_r3data << 3 | 3), %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	// restore the registers
	mov $_sysexit_regs, %rsp
	pop %rdi
	pop %rsi
	pop %rbp
	add $8, %rsp
	pop %rbx
	pop %rdx
	pop %rcx
	pop %rax

	// enable paging
	// %rsp used as a scratch register
	mov (pagedir_current), %rsp
	mov %rsp, %cr3
	sysexit

sysenter_stage1:
	cli /* prevent random IRQs in the middle of kernel code */
	xchgw %bx, %bx

	//   disable paging
	// I don't want to damage any of the registers passed in by the user,
	// so i'm using ESP as a temporary register. At this point there's nothing
	// useful in it, it's == _bss_end.
	mov %cr0, %rsp
	and $0x7FFFFFFF, %rsp  // disable paging
	mov %rsp, %cr0

	// save the registers
	mov $(_sysexit_regs + 64), %rsp
	push %rax
	push %rcx
	push %rdx
	push %rbx
	push $0x0 // pushal pushes %rsp here, but that's worthless
	push %rbp
	push %rsi
	push %rdi

	mov $_bss_end, %rsp
	jmp sysenter_stage2

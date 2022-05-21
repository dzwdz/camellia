/* arch/i386/gdt.c */
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
	xor %edx, %edx

	mov $(SEG_r0code << 3), %eax
	mov $IA32_SYSENTER_CS, %ecx
	wrmsr

	mov $IA32_SYSENTER_ESP, %ecx
	mov $0, %eax // unused
	wrmsr

	mov $IA32_SYSENTER_EIP, %ecx
	mov $sysenter_stage1, %eax
	wrmsr

	ret


.section .text.early

.global stored_eax
stored_eax:
.long 0

.global _sysexit_real
.type _sysexit_real, @function
_sysexit_real:
	mov $(SEG_r3data << 3 | 3), %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	// restore the registers
	mov $_sysexit_regs, %esp
	popal

	// enable paging
	mov %eax, stored_eax
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0
	mov stored_eax, %eax

	sysexit

sysenter_stage1:
	cli /* prevent random IRQs in the middle of kernel code */

	//   disable paging
	// I don't want to damage any of the registers passed in by the user,
	// so i'm using ESP as a temporary register. At this point there's nothing
	// useful in it, it's == _bss_end.
	mov %cr0, %esp
	and $0x7FFFFFFF, %esp  // disable paging
	mov %esp, %cr0

	// save the registers
	mov $(_sysexit_regs + 32), %esp
	pushal

	mov $_bss_end, %esp
	jmp sysenter_stage2

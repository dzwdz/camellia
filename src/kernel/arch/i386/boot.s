.section .text
.global _start
.type _start, @function
_start:
	mov $_stack_top, %esp
	call sysenter_setup
	push %ebx // address of the Multiboot struct
	call kmain_early

.global cpu_shutdown
.type cpu_shutdown, @function
cpu_shutdown:
/* This quits QEMU. While I couldn't find this officially documented anywhere,
 * it is used by QEMU in tests/tcg/i386/system/boot.S (as of commit 40d6ee), so
 * I assume that this is safe-ish to use */
	mov $0x604, %edx
	mov $0x2000, %eax
	outw %ax, %dx

.global halt_cpu
.type halt_cpu, @function
halt_cpu:
	cli
1:	hlt
	jmp 1b


.global cpu_pause
.type cpu_pause, @function
cpu_pause:
	sti
	hlt
	cli
	ret

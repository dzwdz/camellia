.section .text
.global _start
.type _start, @function
_start:
	mov $_stack_top, %esp
	push %ebx // save the address of the multiboot struct

	mov $0x80000000, %eax // check CPUID extended functions
	cpuid
	cmp $0x80000001, %eax
	jb panic_early

	mov $0x80000001, %eax
	cpuid
	test $(1<<29), %edx // check long mode support
	jz panic_early

	mov %cr4, %eax
	or $(1<<5), %eax // PAE
	mov %eax, %cr4

	call pml4_identity_init
	mov $pml4_identity, %eax
	mov %eax, %cr3

	mov $0xC0000080, %ecx // EFER MSR
	rdmsr
	or $(1 | 1<<8 | 1<<11), %eax // syscall/ret | long mode | NX
	wrmsr

	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0

	call gdt_init
	lgdt (lgdt_arg)

	pop %edi

	// TODO import gdt.h for globals
	mov $(2<<3), %eax
	mov %eax, %ds // SEG_r0data
	mov %eax, %ss
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs

	ljmp $(1<<3), $boot64 // SEG_r0code

panic_early:
	// output a vga Fuck
	movl $0x4F754F46, 0xB872A
	movl $0x4F6B4F63, 0xB872E
	jmp cpu_halt

// TODO not part of anything yet
	call sysenter_setup
	// TODO will fail
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

.global cpu_halt
.type cpu_halt, @function
cpu_halt:
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

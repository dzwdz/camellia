.global boot64
boot64:
	lgdt (lgdt_arg) // try reloading gdt again
	mov $(6 << 3 | 3), %ax // SEG_TSS
	ltr %ax

	push %rdi // preserve multiboot struct
	call sysenter_setup
	pop %rdi

	// multiboot struct in %rdi
	jmp kmain_early

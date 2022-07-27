.section .text
.global _start
.type _start, @function
.weak _start
_start:
	mov %rsp, %rbp
	and $~0xF, %rsp
	call elf_selfreloc

	mov %rbp, %rsp
	/* pushed by _freejmp */
	pop %rdi
	pop %rsi
	pop %rdx
	and $~0xF, %rsp
	call main
	mov %rax, %rdi
	jmp exit

.section .text
.global _syscall
.type _syscall, @function
_syscall:
	push %r10
	mov %rcx, %r10
	syscall
	pop %r10
	ret

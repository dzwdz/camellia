.section .text
.global _syscall
.type _syscall, @function
_syscall:
	push %rbx // preserve registers
	push %rsi
	push %rdi
	push %rbp

	// NOT the calling convention TODO you lazy fuck
	mov 20(%rsp), %rax
	mov 24(%rsp), %rbx
	mov %rsp, %rcx
	mov $_syscall_ret, %rdx
	mov 28(%rsp), %rsi
	mov 32(%rsp), %rdi
	mov 36(%rsp), %rbp
	sysenter

_syscall_ret:
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rbx
	ret

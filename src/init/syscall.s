.section .text
.global _syscall
.type _syscall, @function
_syscall:
	push %ebx // preserve registers
	push %esi
	push %edi

//  note: i could squeeze out another parameter out of %ebp
	mov 16(%esp), %eax
	mov 20(%esp), %ebx
	mov %esp, %ecx
	mov $_syscall_ret, %edx
	mov 24(%esp), %esi
	mov 28(%esp), %edi
	sysenter
_syscall_ret:
	pop %edi
	pop %esi
	pop %ebx
	ret

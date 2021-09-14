.section .text
.global _syscall
.type _syscall, @function
_syscall:
	push %ebx // preserve registers
	push %esi
	push %edi
	push %ebp

//  note: i could squeeze out another parameter out of %ebp
	mov 20(%esp), %eax
	mov 24(%esp), %ebx
	mov %esp, %ecx
	mov $_syscall_ret, %edx
	mov 28(%esp), %esi
	mov 32(%esp), %edi
	mov 36(%esp), %ebp
	sysenter
_syscall_ret:
	pop %ebp
	pop %edi
	pop %esi
	pop %ebx
	ret

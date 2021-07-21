.section .text
.global _syscall
.type _syscall, @function
_syscall:
	mov 4(%esp), %eax
	mov 8(%esp), %ebx
	mov %esp, %ecx
	mov $_syscall_ret, %edx
	mov 12(%esp), %esi
	mov 16(%esp), %edi
	sysenter
_syscall_ret:
	ret

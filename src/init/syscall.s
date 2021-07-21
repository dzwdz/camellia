.section .text
.global _syscall
.type _syscall, @function
_syscall:
	mov %esp, %ecx
	mov $_syscall_ret, %edx
	sysenter
_syscall_ret:
	ret

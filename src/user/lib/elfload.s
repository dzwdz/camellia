.section .text
.global _freejmp_chstack
.type _freejmp_chstack, @function
// void _freejmp_chstack(void *entry, void *low, size_t len, char **argv, char **envp, void *stack);
_freejmp_chstack:
	mov %r9, %rsp
	jmp _freejmp

.section .text
.global execbuf_chstack
.type execbuf_chstack, @function
// _Noreturn void execbuf_chstack(void *stack, void __user *buf, size_t len);
execbuf_chstack:
	mov %rdi, %rsp
	mov $100, %rdi
	syscall
	jmp 0 // if execbuf failed we might as well crash
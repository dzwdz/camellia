#define ASM_FILE 1
#include <camellia/syscalls.h>
#include <camellia/flags.h>

.section .text
.global thread_creates
.type thread_creates, @function
// void thread_creates(int flags, void (*fn)(void*), void *arg, void *stack);
thread_creates:
	push %r12
	push %r13
	push %r14

	/* save fn, arg, stack */
	mov %rsi, %r12
	mov %rdx, %r13
	mov %rcx, %r14

	mov %rdi, %rsi
	or $(FORK_SHAREMEM | FORK_SHAREHANDLE), %rsi
	mov $_SYS_FORK, %rdi
	xor %rdx, %rdx
	syscall

	test %rax, %rax
	jz 1f
	/* in parent, return normally */
	pop %r14
	pop %r13
	pop %r12
	ret
1:	/* in child */
	mov %r14, %rsp
	mov %r13, %rdi
	call *%r12

	mov $_SYS_EXIT, %rdi
	xor %rsi, %rsi
	syscall
	hlt /* if all else fails... */

.section .text
.global setjmp
.type setjmp, @function
// int setjmp(jmp_buf env);
setjmp:
	mov  %rbx,  0(%rdi)
	mov  %rbp, 16(%rdi)
	mov  %r12, 24(%rdi)
	mov  %r13, 32(%rdi)
	mov  %r14, 40(%rdi)
	mov  %r15, 48(%rdi)
	/* save registers as if after a ret */
	lea 8(%rsp), %rax
	mov %rax, 8(%rdi)
	mov (%rsp), %rax
	mov %rax, 56(%rdi)
	xor %rax, %rax

	mov   8(%rdi), %rsp
	jmp *56(%rdi)

	ret


.global longjmp
.type longjmp, @function
// _Noreturn void longjmp(jmp_buf env, int val);
longjmp:
	mov %rsi, %rax
	cmp $1, %rax /* carry set as for %rax - 1 - so, 1 only if %rax == 0 */
	adc $0, %rax
1:	mov   0(%rdi), %rbx
	mov   8(%rdi), %rsp
	mov  16(%rdi), %rbp
	mov  24(%rdi), %r12
	mov  32(%rdi), %r13
	mov  40(%rdi), %r14
	mov  48(%rdi), %r15
	jmp *56(%rdi)

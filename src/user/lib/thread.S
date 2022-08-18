.section .text
.global chstack
.type chstack, @function
// _Noreturn void chstack(void *arg, void (*fn)(void*), void *esp);
chstack:
	mov %rdx, %rsp
	call *%rsi
	jmp 0 // "exit"

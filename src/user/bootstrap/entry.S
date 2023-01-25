#define ASM_FILE 1
#include <camellia/syscalls.h>
#include <camellia/flags.h>

.set STACK_TOP, 0xFFFFFFFFFFFFFFFF
.set STACK_PAGES, 4

.section .text.startup
.global _start
.type _start, @function
_start:
	mov $_SYS_MEMFLAG, %rdi
	mov $(STACK_TOP & ~0xFFF - (STACK_PAGES - 1) * 0x1000), %rsi
	mov $(STACK_PAGES * 0x1000), %rdx
	mov $MEMFLAG_PRESENT, %r10
	syscall
	mov $(STACK_TOP & ~0xF), %rsp

	mov $_SYS_MEMFLAG, %rdi
	mov $_bss_start, %rsi
	mov $_bss_end, %rdx
	sub $_bss_start, %rdx
	mov $MEMFLAG_PRESENT, %r10
	mov %rsp, %r8
	syscall

	jmp main

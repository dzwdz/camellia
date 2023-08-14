.section .text
.global _start
.type _start, @function
.weak _start
_start:
	mov %rsp, %rdi
	and $~0xF, %rsp

	/* prevent floating point crashes. thanks heat */
	push $0x1f80
	ldmxcsr (%rsp)
	add $8, %rsp

	call _start2
	hlt
	/* the call shouldn't return, thus the hlt.
	 * using a call instead of jmp for stack alignment */

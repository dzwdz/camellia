.section .text
.global _start
.type _start, @function
.weak _start
_start:
	mov %rsp, %rdi
	and $~0xF, %rsp
	call _start2
	hlt
	/* the call shouldn't return, thus the hlt.
	 * using a call instead of jmp for stack alignment */

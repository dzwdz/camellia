#pragma once
#include <camellia/types.h>
#include <stdint.h>

/* requires 16-byte alignment */
struct registers {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rdi, rsi;
	userptr_t rbp, rsp;
	uint64_t rbx, rdx, rcx, rax;
	uint8_t _sse[512];
} __attribute__((__packed__));

// saves a return value according to the SysV ABI
static inline uint64_t regs_savereturn(struct registers *regs, uint64_t value) {
	regs->rax = value;
	return value;
}

#pragma once
#include <shared/types.h>
#include <stdint.h>

struct registers {
	uint64_t edi, esi;
	userptr_t ebp, esp;
	uint64_t ebx, edx, ecx, eax;

	userptr_t eip;
} __attribute__((__packed__));

// saves a return value according to the SysV ABI
static inline uint64_t regs_savereturn(struct registers *regs, uint64_t value) {
	regs->eax = value;
	regs->edx = value >> 32; // TODO check ABI
	return value;
}

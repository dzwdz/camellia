#pragma once
#include <shared/types.h>
#include <stdint.h>

struct registers {
	/* those are in the order of pushad/popad - so you can load/save this
	 * struct in (almost) one instruction */
	uint32_t edi, esi;
	userptr_t ebp, esp;
	uint32_t ebx, edx, ecx, eax;

	// not part of pushad/popad, but still useful
	userptr_t eip;
} __attribute__((__packed__));

// saves a return value according to the SysV ABI
static inline uint64_t regs_savereturn(struct registers *regs, uint64_t value) {
	regs->eax = value;
	regs->edx = value >> 32;
	return value;
}

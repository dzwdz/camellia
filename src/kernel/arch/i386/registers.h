#pragma once
#include <stdint.h>

struct registers {
	/* those are in the order of pushad/popad - so you can load/save this
	 * struct in (almost) one instruction */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;

	// not part of pushad/popad, but still useful
	uint32_t eip;
} __attribute__((__packed__));

// saves a return value according to the SysV ABI
inline void regs_savereturn(struct registers *regs, uint64_t value) {
	regs->eax = value;
	regs->edx = value >> 32;
}

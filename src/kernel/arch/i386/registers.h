#pragma once
#include <stdint.h>

struct registers {
	uint32_t eax, ebx, ecx, edx, esi, edi;
	void *ebp, *esp, *eip;
};

// saves a return value according to the SysV ABI
inline void regs_savereturn(struct registers *regs, uint64_t value) {
	regs->eax = value;
	regs->edx = value >> 32;
}

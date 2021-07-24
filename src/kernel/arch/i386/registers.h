#pragma once
#include <stdint.h>

// TODO merge registers and registers_pushad

struct registers {
	uint32_t eax, ebx, ecx, edx, esi, edi;
	void *ebp, *esp, *eip;
};

struct registers_pushad {
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} __attribute__((__packed__));

// saves a return value according to the SysV ABI
inline void regs_savereturn(struct registers *regs, uint64_t value) {
	regs->eax = value;
	regs->edx = value >> 32;
}

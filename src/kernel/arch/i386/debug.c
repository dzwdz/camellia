#include <kernel/arch/generic.h>

void *debug_caller(size_t depth) {
	void **ebp;
	asm("mov %%ebp, %0" 
	    : "=r" (ebp));
	while (depth--) {
		if (!ebp) return NULL;
		ebp = *ebp;
	}
	return ebp[1];
}

void debug_stacktrace(void) {
	for (size_t i = 0; i < 16; i++) {
		kprintf("  k/%08x\n", (uintptr_t)debug_caller(i));
	}
}

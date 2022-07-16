#include <kernel/arch/generic.h>

void *debug_caller(size_t depth) {
	void **rbp;
	asm("mov %%rbp, %0" 
	    : "=r" (rbp));
	while (depth--) {
		if (!rbp) return NULL;
		rbp = *rbp;
	}
	return rbp[1];
}

void debug_stacktrace(void) {
	for (size_t i = 0; i < 16; i++) {
		kprintf("  k/%08x\n", (uintptr_t)debug_caller(i));
	}
}

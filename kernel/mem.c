#include <kernel/mem.h>

extern void *_kernel_end;
static void *highest;

void mem_init() {
	highest = &_kernel_end;
}

// should always succeed, there are no error checks anywhere
void *malloc(size_t size) {
	void *block = highest;
	highest += size;
	return block;
}

void free(void *ptr) {
	// not implemented
}

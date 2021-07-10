#include <arch/generic.h>
#include <kernel/mem.h>

extern void *_kernel_end;
static void *highest_page;

void mem_init() {
	highest_page = &_kernel_end;
}

void *page_alloc(size_t pages) {
	void *bottom = highest_page;
	highest_page += pages * PAGE_SIZE;
	return bottom;
}

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages) {
	// not implemented
}

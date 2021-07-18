#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <stdint.h>

static void *highest_page;

void mem_init(struct kmain_info *info) {
	// finds the highest used page, and starts allocating pages above it
	extern char _bss_end;
	void *highest = &_bss_end;
	size_t page_mask = PAGE_SIZE - 1;

	if (highest < info->init.at + info->init.size)
		highest = info->init.at + info->init.size;

	// align up to PAGE_SIZE
	highest_page = (void*)(((uintptr_t)highest + page_mask) & ~page_mask);
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

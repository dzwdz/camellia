#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/util.h>
#include <stdint.h>

static void *highest_page;
static int malloc_balance = 0;

void mem_init(struct kmain_info *info) {
	// finds the highest used page, and starts allocating pages above it
	void *highest = &_bss_end;

	if (highest < info->init.at + info->init.size)
		highest = info->init.at + info->init.size;

	// align up to PAGE_SIZE
	highest_page = (void*)(((uintptr_t)highest + PAGE_MASK) & ~PAGE_MASK);
}

void mem_debugprint(void) {
	tty_const("malloc balance: ");
	_tty_var(malloc_balance);
	tty_const(" ");
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


void *kmalloc(size_t len) {
	malloc_balance++;
	// extremely inefficient, but this is only temporary anyways
	return page_alloc(len / PAGE_SIZE + 1);
}

void kfree(void *ptr) {
	if (ptr == NULL) return;
	malloc_balance--;
}

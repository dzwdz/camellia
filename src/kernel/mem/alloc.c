#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/util.h>
#include <stdint.h>

#define MALLOC_MAGIC 0xACAB1312

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
	void *addr;
	len += sizeof(uint32_t); // add space for MALLOC_MAGIC
	// extremely inefficient, but this is only temporary anyways
	addr = page_alloc(len / PAGE_SIZE + 1);
	*(uint32_t*)addr = MALLOC_MAGIC;
	malloc_balance++;
	return addr + sizeof(uint32_t);
}

void kfree(void *ptr) {
	uint32_t *magic = &((uint32_t*)ptr)[-1];
	if (ptr == NULL) return;
	if (*magic != MALLOC_MAGIC) {
		// TODO add some kind of separate system log
		tty_const("WARNING kfree() didn't find MALLOC_MAGIC, ptr == ");
		_tty_var(ptr);
		tty_const(" ");
	} else {
		// change the magic marker to detect double frees
		*magic = 0xBADF2EED;
	}
	malloc_balance--;
}

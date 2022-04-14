#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/util.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <stdint.h>

#define MALLOC_MAGIC 0xACAB1312

static void *memtop;

static uint8_t *page_bitmap;
static void *page_bitmap_start;
static size_t page_bitmap_len;

static int malloc_balance = 0;

static void *closest_page(void *p) {
	return (void*)(((uintptr_t)p + PAGE_MASK) & ~PAGE_MASK);
}

void mem_init(struct kmain_info *info) {
	memtop = info->memtop;

	/* place the page_bitmap right at the first free spot in memory */
	page_bitmap = max((void*)&_bss_end, closest_page(info->init.at + info->init.size));
	page_bitmap_len = ((uintptr_t)memtop - (uintptr_t)page_bitmap) / PAGE_SIZE / 8;
	/* the page_bitmap_len calculation doesn't account for the space lost to itself,
	 * so just take away a few pages to account for it. this could be solved with
	 * actual math, but eh. */
	page_bitmap_len -= 8;
	memset(page_bitmap, 0, page_bitmap_len);

	/* start allocation on the page immediately following the page bitmap */
	page_bitmap_start = closest_page(page_bitmap + page_bitmap_len);
}

void mem_debugprint(void) {
	tty_const("malloc balance: ");
	_tty_var(malloc_balance);
	tty_const(" ");
}

static bool bitmap_get(size_t i) {
	size_t b = i / 8;
	assert(b < page_bitmap_len);
	return 0 < (page_bitmap[b] & (1 << (i&7)));
}

static void bitmap_set(size_t i, bool v) {
	size_t b = i / 8;
	uint8_t m = 1 << (i&7);
	assert(b < page_bitmap_len);
	if (v) page_bitmap[b] |=  m;
	else   page_bitmap[b] &= ~m;
}

void *page_alloc(size_t pages) {
	/* i do realize how painfully slow this is */
	size_t streak = 0;
	for (size_t i = 0; i < page_bitmap_len * 8; i++) {
		if (bitmap_get(i)) {
			streak = 0;
			continue;
		}
		if (++streak >= pages) {
			/* found hole big enough for this allocation */
			i = i + 1 - streak;
			for (size_t j = 0; j < streak; j++)
				bitmap_set(i + j, true);
			return page_bitmap_start + i * PAGE_SIZE;
		}
	}
	tty_const("we ran out of memory :(\ngoodbye.\n");
	panic_unimplemented();
}

// frees `pages` consecutive pages starting from *first
void page_free(void *first_addr, size_t pages) {
	size_t first = (uintptr_t)(first_addr - page_bitmap_start) / PAGE_SIZE;
	for (size_t i = 0; i < pages; i++) {
		assert(bitmap_get(first + i));
		bitmap_set(first + i, false);
	}
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

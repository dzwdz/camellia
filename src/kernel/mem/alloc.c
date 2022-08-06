#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/util.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <stdint.h>

#define MALLOC_MAGIC 0xACAB1312

struct malloc_hdr {
	uint32_t magic;
	uint32_t page_amt;
	struct malloc_hdr *next, *prev;
	void *stacktrace[4];
};

struct malloc_hdr *malloc_last = NULL;

extern uint8_t pbitmap[], pbitmap_start[]; /* linker.ld */
static size_t pbitmap_len;


static bool bitmap_get(size_t i) {
	size_t b = i / 8;
	assert(b < pbitmap_len);
	return 0 < (pbitmap[b] & (1 << (i&7)));
}

static void bitmap_set(size_t i, bool v) {
	size_t b = i / 8;
	uint8_t m = 1 << (i&7);
	assert(b < pbitmap_len);
	if (v) pbitmap[b] |=  m;
	else   pbitmap[b] &= ~m;
}

static void bitmap_mark(void *start, size_t len) {
	/* align to the previous page */
	size_t off = (uintptr_t)start & PAGE_MASK;
	start -= off;
	len += off;
	size_t first = ((uintptr_t)start - (uintptr_t)pbitmap_start) / PAGE_SIZE;
	for (size_t i = 0; i * PAGE_SIZE < len; i++) {
		if (first + i >= pbitmap_len) break;
		bitmap_set(first + i, true);
	}
}

void mem_init(struct kmain_info *info) {
	size_t pageamt = ((uintptr_t)info->memtop - (uintptr_t)pbitmap_start) / PAGE_SIZE;
	pbitmap_len = pageamt / 8;
	memset(pbitmap, 0, pbitmap_len);
	bitmap_mark(pbitmap, pbitmap_len);
	bitmap_mark(info->init.at, info->init.size);
	bitmap_mark(info->fb.at,   info->fb.size);
}

void mem_debugprint(void) {
	size_t total = 0;
	kprintf("current kmallocs:\n");
	kprintf("addr      pages\n");
	for (struct malloc_hdr *iter = malloc_last; iter; iter = iter->prev) {
		kprintf("%08x  %05x", iter, iter->page_amt);
		for (size_t i = 0; i < 4; i++)
			kprintf("  k/%08x", iter->stacktrace[i]);
		kprintf("\n    peek 0x%x\n", *(uint32_t*)(&iter[1]));
		total++;
	}
	kprintf("  total 0x%x\n", total);
}

void *page_alloc(size_t pages) {
	/* i do realize how painfully slow this is */
	size_t streak = 0;
	for (size_t i = 0; i < pbitmap_len * 8; i++) {
		if (bitmap_get(i)) {
			streak = 0;
			continue;
		}
		if (++streak >= pages) {
			/* found hole big enough for this allocation */
			i = i + 1 - streak;
			for (size_t j = 0; j < streak; j++)
				bitmap_set(i + j, true);
			return pbitmap_start + i * PAGE_SIZE;
		}
	}
	kprintf("we ran out of memory :(\ngoodbye.\n");
	panic_unimplemented();
}

void *page_zalloc(size_t pages) {
	void *p = page_alloc(pages);
	memset(p, 0, pages * PAGE_SIZE);
	return p;
}

// frees `pages` consecutive pages starting from *first
void page_free(void *first_addr, size_t pages) {
	assert(first_addr >= (void*)pbitmap_start);
	size_t first = ((uintptr_t)first_addr - (uintptr_t)pbitmap_start) / PAGE_SIZE;
	for (size_t i = 0; i < pages; i++) {
		assert(bitmap_get(first + i));
		bitmap_set(first + i, false);
	}
}

void kmalloc_sanity(const void *addr) {
	assert(addr);
	const struct malloc_hdr *hdr = addr - sizeof(struct malloc_hdr);
	assert(hdr->magic == MALLOC_MAGIC);
	if (hdr->next) assert(hdr->next->prev == hdr);
	if (hdr->prev) assert(hdr->prev->next == hdr);
}

void *kmalloc(size_t len) {
	// TODO better kmalloc

	struct malloc_hdr *hdr;
	void *addr;
	uint32_t pages;

	len += sizeof(struct malloc_hdr);
	pages = len / PAGE_SIZE + 1;

	hdr = page_alloc(pages);
	hdr->magic = MALLOC_MAGIC;
	hdr->page_amt = pages;

	hdr->next = NULL;
	hdr->prev = malloc_last;
	if (hdr->prev) {
		assert(!hdr->prev->next);
		hdr->prev->next = hdr;
	}

	for (size_t i = 0; i < 4; i++)
		hdr->stacktrace[i] = debug_caller(i);

	malloc_last = hdr;

	addr = (void*)hdr + sizeof(struct malloc_hdr);
	kmalloc_sanity(addr);
	return addr;
}

void kfree(void *ptr) {
	struct malloc_hdr *hdr;
	if (ptr == NULL) return;

	hdr = ptr - sizeof(struct malloc_hdr);
	kmalloc_sanity(ptr);

	hdr->magic = ~MALLOC_MAGIC; // (hopefully) detect double frees
	if (hdr->next)
		hdr->next->prev = hdr->prev;
	if (hdr->prev)
		hdr->prev->next = hdr->next;
	if (malloc_last == hdr)
		malloc_last = hdr->prev;
	page_free(hdr, hdr->page_amt);
}

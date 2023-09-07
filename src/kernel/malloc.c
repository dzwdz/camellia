#include <kernel/arch/generic.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/util.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <stdint.h>

#define MALLOC_MAGIC 0xA770C666

struct malloc_hdr {
	uint32_t magic;
	uint32_t page_amt;
	struct malloc_hdr *next, *prev;
	void *stacktrace[4];
};

struct malloc_hdr *malloc_last = NULL;

extern uint8_t pbitmap[]; /* linker.ld */
static size_t pbitmap_len; /* in bytes */
static size_t pbitmap_firstfree = 0;


static bool bitmap_get(long i) {
	assert(i >= 0);
	size_t b = i / 8;
	uint8_t m = 1 << (i&7);
	assert(b < pbitmap_len);
	return (pbitmap[b]&m) != 0;
}

static bool bitmap_set(long i, bool v) {
	assert(i >= 0);
	size_t b = i / 8;
	uint8_t m = 1 << (i&7);
	assert(b < pbitmap_len);
	bool prev = (pbitmap[b]&m) != 0;
	if (v) pbitmap[b] |=  m;
	else   pbitmap[b] &= ~m;
	return prev;
}

static long toindex(void *p) {
	return ((long)p - (long)pbitmap) / PAGE_SIZE;
}

void mem_init(void *memtop) {
	kprintf("memory   %8x -> %8x\n", &_bss_end, memtop);
	pbitmap_len = toindex(memtop) / 8;
	memset(pbitmap, 0, pbitmap_len);
	mem_reserve(pbitmap, pbitmap_len);
}

void mem_reserve(void *addr, size_t len) {
	kprintf("reserved %8x -> %8x\n", addr, addr + len);

	/* align to the previous page */
	size_t off = (uintptr_t)addr & PAGE_MASK;
	addr -= off;
	len += off;
	size_t first = toindex(addr);
	for (size_t i = 0; i * PAGE_SIZE < len; i++) {
		if ((first + i) / 8 >= pbitmap_len)
			break;
		bitmap_set(first + i, true);
	}
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
	for (size_t i = pbitmap_firstfree; i < pbitmap_len * 8; i++) {
		if (bitmap_get(i)) {
			streak = 0;
			continue;
		}
		if (++streak >= pages) {
			/* found hole big enough for this allocation */
			i = i + 1 - streak;
			for (size_t j = 0; j < streak; j++)
				bitmap_set(i + j, true);
			pbitmap_firstfree = i + streak - 1;
			return pbitmap + i * PAGE_SIZE;
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
void page_free(void *addr, size_t pages) {
	assert(addr >= (void*)pbitmap);
	size_t first = toindex(addr);
	for (size_t i = 0; i < pages; i++) {
		if (bitmap_set(first + i, false) == false) {
			panic_invalid_state();
		}
	}
	if (pbitmap_firstfree > first) {
		pbitmap_firstfree = first;
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
#ifndef NDEBUG
	memset(addr, 0xCC, len);
#endif
	kmalloc_sanity(addr);
	return addr;
}

void kfree(void *ptr) {
	struct malloc_hdr *hdr;
	uint32_t page_amt;
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
	page_amt = hdr->page_amt;
#ifndef NDEBUG
	memset(hdr, 0xC0, page_amt * PAGE_SIZE);
#endif
	page_free(hdr, page_amt);
}

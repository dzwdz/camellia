#pragma once
#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <stddef.h>

extern struct malloc_hdr *malloc_last;

void mem_init(struct kmain_info *);
void mem_debugprint(void);

// allocates `pages` consecutive pages
// TODO deprecate
void *page_alloc(size_t pages);
// zeroes the allocated pages
void *page_zalloc(size_t pages);

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages);

void kmalloc_sanity(const void *addr);
void *kmalloc(size_t len);
void kfree(void *ptr);

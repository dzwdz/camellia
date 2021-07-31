#pragma once
#include <kernel/main.h>
#include <stddef.h>

void mem_init(struct kmain_info *);

// allocates `pages` consecutive pages
void *page_alloc(size_t pages);

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages);

void *kmalloc(size_t len);
void kfree(void *ptr);

#pragma once
#include <stddef.h>

#define PAGE_SIZE 4096

void mem_init();

// allocates `pages` consecutive pages
void *page_alloc(size_t pages);

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages);

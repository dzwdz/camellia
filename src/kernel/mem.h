#pragma once
#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <stddef.h>

void mem_init(struct kmain_info *);

// allocates `pages` consecutive pages
void *page_alloc(size_t pages);

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages);

void *kmalloc(size_t len);
void kfree(void *ptr);


// used for iterating through some part of a process' memory
struct virt_iter {
	void *frag;
	size_t frag_len;
	size_t prior; // sum of all prior frag_lens
	bool error;

	void *_virt;
	size_t _remaining;
	struct pagedir *_pages;
	bool _user;
	bool _writeable;
};

void virt_iter_new(
		struct virt_iter *iter, void *virt, size_t length,
		struct pagedir *pages, bool user, bool writeable);

bool virt_iter_next(struct virt_iter *);

bool virt_user_cpy(
		struct pagedir *dest_pages,       void *dest,
		struct pagedir  *src_pages, const void *src, size_t length);

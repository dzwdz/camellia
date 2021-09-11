/* contains utilities for interacting with virtual memory */
#pragma once
#include <kernel/types.h>
#include <stdbool.h>
#include <stddef.h>

struct virt_iter {
	void *frag;
	size_t frag_len;
	size_t prior; // sum of all prior frag_lens
	bool error;

	void __user *_virt;
	size_t _remaining;
	struct pagedir *_pages;
	bool _user;
	bool _writeable;
};

/* if pages == NULL, create an iterator over physical memory. */
void virt_iter_new(
		struct virt_iter *iter, void __user *virt, size_t length,
		struct pagedir *pages, bool user, bool writeable);

bool virt_iter_next(struct virt_iter *);

bool virt_cpy(
		struct pagedir *dest_pages,       void __user *dest,
		struct pagedir  *src_pages, const void __user *src, size_t length);

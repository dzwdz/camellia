/* contains utilities for interacting with virtual memory */
#pragma once
#include <kernel/mem/alloc.h>
#include <shared/types.h>
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


static inline bool virt_cpy_to(struct pagedir *dest_pages, // physical -> virtual
		void __user *dest, const void *src, size_t length) {
	return virt_cpy(dest_pages, dest, NULL, (userptr_t)src, length);
}

static inline bool virt_cpy_from(struct pagedir *src_pages, // virtual -> physical
		void *dest, const void __user *src, size_t length) {
	return virt_cpy(NULL, (userptr_t)dest, src_pages, src, length);
}

/** Copies a chunk of virtual memory to a newly kmalloc'd buffer. */
static inline void *virt_cpy2kmalloc(struct pagedir *src_pages,
		const void __user *src, size_t length) {
	void *buf = kmalloc(length);
	if (virt_cpy_from(src_pages, buf, src, length)) {
		return buf;
	} else {
		kfree(buf);
		return NULL;
	}
}

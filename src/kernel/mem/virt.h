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

struct virt_cpy_error {
	bool read_fail, write_fail;
};

/* if pages == NULL, create an iterator over physical memory. */
void virt_iter_new(
		struct virt_iter *iter, void __user *virt, size_t length,
		struct pagedir *pages, bool user, bool writeable);

bool virt_iter_next(struct virt_iter *);

size_t virt_cpy(
		struct pagedir *dest_pages,       void __user *dest,
		struct pagedir  *src_pages, const void __user *src,
		size_t length, struct virt_cpy_error *err);


/* copies to virtual memory, returns true on success */
static inline bool virt_cpy_to(struct pagedir *dest_pages,
		void __user *dest, const void *src, size_t length) {
	return length == virt_cpy(dest_pages, dest, NULL, (userptr_t)src, length, NULL);
}

/* copies from virtual memory, returns true on success */
static inline bool virt_cpy_from(struct pagedir *src_pages,
		void *dest, const void __user *src, size_t length) {
	return length == virt_cpy(NULL, (userptr_t)dest, src_pages, src, length, NULL);
}

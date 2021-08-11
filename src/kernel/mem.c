#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <stdint.h>

static void *highest_page;

void mem_init(struct kmain_info *info) {
	// finds the highest used page, and starts allocating pages above it
	void *highest = &_bss_end;

	if (highest < info->init.at + info->init.size)
		highest = info->init.at + info->init.size;

	// align up to PAGE_SIZE
	highest_page = (void*)(((uintptr_t)highest + PAGE_MASK) & ~PAGE_MASK);
}

void *page_alloc(size_t pages) {
	void *bottom = highest_page;
	highest_page += pages * PAGE_SIZE;
	return bottom;
}

// frees `pages` consecutive pages starting from *first
void page_free(void *first, size_t pages) {
	// not implemented
}


void *kmalloc(size_t len) {
	// extremely inefficient, but this is only temporary anyways
	return page_alloc(len / PAGE_SIZE + 1);
}

void kfree(void *ptr) {
	// unimplemented
}


// TODO move to some shared file in kernel/arch/
void virt_iter_new(
		struct virt_iter *iter, void *virt, size_t length,
		struct pagedir *pages, bool user, bool writeable)
{
	iter->frag       = 0;
	iter->error      = false;
	iter->_virt      = virt;
	iter->_remaining = length;
	iter->_pages     = pages;
	iter->_user      = user;
	iter->_writeable = writeable;
}

bool virt_iter_next(struct virt_iter *iter) {
	/* note: While i'm pretty sure that this should always work, this
	 * was only tested in cases where the pages were consecutive both in
	 * virtual and physical memory, which might not always be the case.
	 * TODO test this */

	uintptr_t virt = (uintptr_t) iter->_virt;
	size_t partial = iter->_remaining;
	if (partial <= 0) return false;

	// don't read past the page
	if ((virt & PAGE_MASK) + partial > PAGE_SIZE)
		partial = PAGE_SIZE - (virt & PAGE_MASK);

	iter->frag = pagedir_virt2phys(iter->_pages,
			iter->_virt, iter->_user, iter->_writeable);

	if (iter->frag == 0) {
		iter->error = true;
		return false;
	}

	iter->frag_len    = partial;
	iter->_remaining -= partial;
	iter->_virt      += partial;
	return true;
}

/* Copies stuff between different pagedirs. Mostly a wrapper behind an old,
 * bad interface. */

// TODO ensure the behaviour of kernel vs user fs on faults is the same

#include <kernel/arch/generic.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <shared/mem.h>

struct virt_iter {
	void *frag;
	size_t frag_len;
	size_t prior; // sum of all prior frag_lens
	bool error;

	void __user *_virt;
	size_t _remaining;
	Pagedir *_pages;
	bool _user;
	bool _writeable;
};

struct virt_cpy_error { // unused
	bool read_fail, write_fail;
};

/* if pages == NULL, creates an iterator over physical memory. */
static void virt_iter_new(
	struct virt_iter *iter, void __user *virt, size_t length,
	Pagedir *pages, bool user, bool writeable
);
static bool virt_iter_next(struct virt_iter *);
static size_t virt_cpy(
	Pagedir *dest_pages,       void __user *dest,
	Pagedir  *src_pages, const void __user *src,
	size_t length, struct virt_cpy_error *err
);


static void
virt_iter_new(
	struct virt_iter *iter, void __user *virt, size_t length,
	Pagedir *pages, bool user, bool writeable
) {
	iter->frag       = NULL;
	iter->frag_len   = 0;
	iter->prior      = 0;
	iter->error      = false;
	iter->_virt      = virt;
	iter->_remaining = length;
	iter->_pages     = pages;
	iter->_user      = user;
	iter->_writeable = writeable;
}

static bool
virt_iter_next(struct virt_iter *iter)
{
	/* note: While i'm pretty sure that this should always work, this
	 * was only tested in cases where the pages were consecutive both in
	 * virtual and physical memory, which might not always be the case.
	 * TODO test this */

	size_t partial = iter->_remaining;
	iter->prior   += iter->frag_len;
	if (partial <= 0) return false;

	if (iter->_pages) { // if iterating over virtual memory
		// don't cross page boundaries
		size_t to_page = PAGE_SIZE - ((uintptr_t)iter->_virt & PAGE_MASK);
		partial = min(partial, to_page);

		iter->frag = pagedir_virt2phys(iter->_pages,
				iter->_virt, iter->_user, iter->_writeable);

		if (!iter->frag) {
			iter->error = true;
			return false;
		}
	} else {
		// "iterate" over physical memory
		// the double cast supresses the warning about changing address spaces
		iter->frag = (void* __force)iter->_virt;
	}

	iter->frag_len    = partial;
	iter->_remaining -= partial;
	iter->_virt      += partial;
	return true;
}

static size_t
virt_cpy(
		Pagedir *dest_pages,       void __user *dest,
		Pagedir  *src_pages, const void __user *src,
		size_t length, struct virt_cpy_error *err
) {
	struct virt_iter dest_iter, src_iter;
	size_t total = 0, partial;

	virt_iter_new(&dest_iter,           dest, length, dest_pages, true, true);
	virt_iter_new( &src_iter, (userptr_t)src, length,  src_pages, true, false);
	dest_iter.frag_len = 0;
	src_iter.frag_len  = 0;

	for (;;) {
		if (dest_iter.frag_len <= 0 && !virt_iter_next(&dest_iter)) break;
		if ( src_iter.frag_len <= 0 && !virt_iter_next( &src_iter)) break;

		partial = min(src_iter.frag_len, dest_iter.frag_len);
		total += partial;
		memcpy(dest_iter.frag, src_iter.frag, partial);

		dest_iter.frag_len -= partial;
		dest_iter.frag     += partial;
		src_iter.frag_len  -= partial;
		src_iter.frag      += partial;
	}

	if (err) {
		err->read_fail  = src_iter.error;
		err->write_fail = dest_iter.error;
	}
	if (src_iter.error || dest_iter.error)
		assert(total != length);
	else
		assert(total == length);
	return total;
}

size_t
pcpy_to(Proc *p, __user void *dst, const void *src, size_t len)
{
	assert(p);
	if (!p->pages) return 0;
	return virt_cpy(p->pages, dst, NULL, (__user void*)src, len, NULL);
}

size_t
pcpy_from(Proc *p, void *dst, const __user void *src, size_t len)
{
	assert(p);
	if (!p->pages) return 0;
	return virt_cpy(NULL, (__user void*)dst, p->pages, src, len, NULL);
}

size_t
pcpy_bi(
	Proc *dstp, __user void *dst,
	Proc *srcp, const __user void *src,
	size_t len
) {
	assert(dstp && srcp);
	if (!dstp->pages) return 0;
	if (!srcp->pages) return 0;
	return virt_cpy(dstp->pages, dst, srcp->pages, src, len, NULL);
}

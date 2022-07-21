#include <kernel/arch/amd64/paging.h>
#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <shared/mem.h>
#include <stdint.h>

static void *addr_extract(pe_generic_t pe) {
	return (void*)(uintptr_t)(pe.address << 12);
}

static void *addr_validate(void *addr) {
	pe_generic_t pe = {.as_ptr = addr};
	assert(addr_extract(pe) == addr);
	return addr;
}

static bool addr_canonical(const __user void *addr) {
	const int addr_bits = 48;
	uintptr_t n = (uintptr_t)addr >> addr_bits;
	return (n == 0) || ((~n) << addr_bits == 0);
}

/* the types here are idiotic because C is idiotic */
static __user void *addr_canonize(const __user void *addr) {
	union virt_addr v = {.full = (void __user*)addr};
	v.sign = (((uintptr_t)addr >> 47) & 1) * 0xFFFF;
	assert(addr_canonical(addr));
	return v.full;
}


struct pagedir *pagedir_new(void) {
	struct pagedir *dir = page_alloc(1);
	memset(dir, 0, sizeof *dir);
	return dir;
}

void pagedir_free(struct pagedir *dir) {
	for (int i = 0; i < 512; i++) {
		if (!dir->e[i].present) continue;
		assert(!dir->e[i].large);
		pe_generic_t *pdpt = addr_extract(dir->e[i]);

		for (int j = 0; j < 512; j++) {
			if (!pdpt[j].present) continue;
			assert(!pdpt[j].large);
			pe_generic_t *pd = addr_extract(pdpt[j]);

			for (int k = 0; k < 512; k++) {
				if (!pd[k].present) continue;
				assert(!pd[k].large);
				pe_generic_t *pt = addr_extract(pd[k]);

				for (int l = 0; l < 512; l++) {
					if (!pt[l].present) continue;
					if (!pt[l].user) continue;
					page_free(addr_extract(pt[l]), 1);
				}
				page_free(pt, 1);
			}
			page_free(pd, 1);
		}
		page_free(pdpt, 1);
	}
	page_free(dir, 1);
}

static pe_generic_t*
get_entry(struct pagedir *dir, const void __user *virt) {
	pe_generic_t *pml4e, *pdpte, *pde, *pte;
	const union virt_addr v = {.full = (void __user *)virt};

	if (!addr_canonical(virt)) return NULL;

	pml4e = &dir->e[v.pml4];
	if (!pml4e->present) return NULL;
	assert(!pml4e->large);

	pdpte = &((pe_generic_t*)addr_extract(*pml4e))[v.pdpt];
	if (!pdpte->present) return NULL;
	assert(!pdpte->large);

	pde = &((pe_generic_t*)addr_extract(*pdpte))[v.pd];
	if (!pde->present) return NULL;
	assert(!pde->large);

	pte = &((pe_generic_t*)addr_extract(*pde))[v.pt];
	return pte;
}

void pagedir_unmap_user(struct pagedir *dir, void __user *virt, size_t len) {
	// TODO rewrite this
	const void __user *end = addr_canonize(virt + len);
	union virt_addr v = {.full = virt};
	v.off_4k = 0;

	for (int pml4i = v.pml4; v.full < end && pml4i < 512; pml4i++) {
		v.pml4 = pml4i;
		v.full = addr_canonize(v.full);
		if (v.full >= end) return;
		if (!dir->e[v.pml4].present) {
			v.pdpt = 0;
			v.pd = 0;
			v.pt = 0;
			continue;
		}
		assert(!dir->e[v.pml4].large);
		pe_generic_t *pdpt = addr_extract(dir->e[v.pml4]);

		for (int pdpti = v.pdpt; v.full < end && pdpti < 512; pdpti++) {
			v.pdpt = pdpti;
			if (v.full >= end) return;
			if (!pdpt[v.pdpt].present) {
				v.pd = 0;
				v.pt = 0;
				continue;
			}
			assert(!pdpt[v.pdpt].large);
			pe_generic_t *pd = addr_extract(pdpt[v.pdpt]);

			for (int pdi = v.pd; v.full < end && pdi < 512; pdi++) {
				v.pd = pdi;
				if (v.full >= end) return;
				if (!pd[v.pd].present) {
					v.pt = 0;
					continue;
				}
				assert(!pd[v.pd].large);
				pe_generic_t *pt = addr_extract(pd[v.pd]);

				for (int pti = v.pt; v.full < end && pti < 512; pti++) {
					v.pt = pti;
					if (v.full >= end) return;
					if (!pt[v.pt].present) continue;
					if (!pt[v.pt].user) continue;
					page_free(addr_extract(pt[v.pt]), 1);
					pt[v.pt].present = 0;
				}
				v.pt = 0;
			}
			v.pd = 0;
		}
		v.pdpt = 0;
	}
}

void pagedir_map(struct pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable)
{
	pe_generic_t *pml4e, *pdpte, *pde, *pte;
	const union virt_addr v = {.full = virt};

	if (!addr_canonical(virt)) return;

	pml4e = &dir->e[v.pml4];
	if (!pml4e->present) {
		pml4e->as_ptr = addr_validate(page_zalloc(1));
		pml4e->present = 1;
		pml4e->writeable = 1;
		pml4e->user = 1;
	}
	assert(!pml4e->large);

	pdpte = &((pe_generic_t*)addr_extract(*pml4e))[v.pdpt];
	if (!pdpte->present) {
		pdpte->as_ptr = addr_validate(page_zalloc(1));
		pdpte->present = 1;
		pdpte->writeable = 1;
		pdpte->user = 1;
	}
	assert(!pdpte->large);

	pde = &((pe_generic_t*)addr_extract(*pdpte))[v.pd];
	if (!pde->present) {
		pde->as_ptr = addr_validate(page_zalloc(1));
		pde->present = 1;
		pde->writeable = 1;
		pde->user = 1;
	}
	assert(!pde->large);

	pte = &((pe_generic_t*)addr_extract(*pde))[v.pt];
	if (!pte->present) {
		pte->address = ((uintptr_t)phys) >> 12;
		pte->present = 1;
		pte->writeable = writeable;
		pte->user = user;
	} else {
		panic_invalid_state();
	}
}

extern void *pagedir_current;
void pagedir_switch(struct pagedir *dir) {
	pagedir_current = dir;
}

// creates a new pagedir with exact copies of the user pages
struct pagedir *pagedir_copy(const struct pagedir *pml4_old) {
	struct pagedir *pml4_new = page_zalloc(1);

	for (int i = 0; i < 512; i++) {
		if (!pml4_old->e[i].present) continue;
		assert(!pml4_old->e[i].large);
		const pe_generic_t *pdpt_old = addr_extract(pml4_old->e[i]);
		pe_generic_t *pdpt_new = page_zalloc(1);
		pml4_new->e[i] = pml4_old->e[i];
		pml4_new->e[i].address = (uintptr_t) pdpt_new >> 12;

		for (int j = 0; j < 512; j++) {
			if (!pdpt_old[j].present) continue;
			assert(!pdpt_old[j].large);
			const pe_generic_t *pd_old = addr_extract(pdpt_old[j]);
			pe_generic_t *pd_new = page_zalloc(1);
			pdpt_new[j] = pdpt_old[j];
			pdpt_new[j].address = (uintptr_t) pd_new >> 12;

			for (int k = 0; k < 512; k++) {
				if (!pd_old[k].present) continue;
				assert(!pd_old[k].large);
				const pe_generic_t *pt_old = addr_extract(pd_old[k]);
				pe_generic_t *pt_new = page_zalloc(1);
				pd_new[k] = pd_old[k];
				pd_new[k].address = (uintptr_t) pt_new >> 12;

				for (int l = 0; l < 512; l++) {
					if (!pt_old[l].present) continue;
					pt_new[l] = pt_old[l];

					if (!pt_old[l].user) continue;
					void *page_new = page_alloc(1);
					memcpy(page_new, addr_extract(pt_old[l]), PAGE_SIZE);
					pt_new[l].address = (uintptr_t) page_new >> 12;
				}
			}
		}
	}

	return pml4_new;
}

bool pagedir_iskern(struct pagedir *dir, const void __user *virt) {
	pe_generic_t *page = get_entry(dir, virt);
	return page && page->present && !page->user;
}

void *pagedir_virt2phys(struct pagedir *dir, const void __user *virt,
                        bool user, bool writeable)
{
	pe_generic_t *page = get_entry(dir, virt);
	if (!page || !page->present) return NULL;
	if (user && !page->user) return NULL;
	if (writeable && !page->writeable) return NULL;

	return addr_extract(*page) + ((uintptr_t)virt & PAGE_MASK);
}

void __user *pagedir_findfree(struct pagedir *dir, char __user *start, size_t len) {
	// TODO dogshit slow
	pe_generic_t *page;
	char __user *iter;
	start = (userptr_t)(((uintptr_t __force)start + PAGE_MASK) & ~PAGE_MASK); // round up to next page
	iter = start;

	while (iter < (char __user *)0xFFF00000) { // TODO better boundary
		page = get_entry(dir, iter);
		if (page && page->present) {
			start = iter + PAGE_SIZE;
		} else {
			if ((size_t)(iter + PAGE_SIZE - start) >= len)
				return start;
		}
		iter += PAGE_SIZE;
	}
	return NULL;
}

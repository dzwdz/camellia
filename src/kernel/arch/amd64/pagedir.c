#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <shared/mem.h>
#include <stdint.h>

/* <heat> nitpick: I highly recommend you dont use bitfields for paging
 *        structures
 * <heat> you can't change them atomically and the way they're layed out
 *        in memory is implementation defined iirc
 */
// TODO move to single shared header file for the 32bit bootstrap
typedef union pe_generic_t {
	struct {
		uint64_t present   : 1;
		uint64_t writeable : 1;
		uint64_t user      : 1;
		uint64_t writethru : 1;

		uint64_t uncached  : 1;
		uint64_t accessed  : 1;
		uint64_t dirty     : 1;
		uint64_t large     : 1; // also PAT

		uint64_t global    : 1; // TODO enable CR4.PGE
		uint64_t _unused2  : 2;
		uint64_t _unused3  : 1; // HLAT

		uint64_t address   : 40;

		uint64_t _unused4  : 7;
		uint64_t pke       : 4;
		uint64_t noexec    : 1;
	} __attribute__((packed));
	void *as_ptr;
} pe_generic_t; // pageentry_generic

struct pagedir { // actually pml4, TODO rename to a more generic name
	pe_generic_t e[512];
} __attribute__((packed));


union virt_addr {
	void __user *full;
	struct {
		uint64_t off_4k : 12; //   4Kb // offset in page
		uint64_t pt     :  9; //   2Mb // page table index
		uint64_t pd     :  9; //   1Gb // page directory index
		uint64_t pdpt   :  9; // 512Gb // page directory pointer table index
		uint64_t pml4   :  9; // 256Tb // page map level 4 index
		uint64_t sign   : 16;
	} __attribute__((packed));
};

static void *addr_extract(pe_generic_t pe) {
	return (void*)(uintptr_t)(pe.address << 12);
}

static void *addr_validate(void *addr) {
	pe_generic_t pe = {.as_ptr = addr};
	assert(addr_extract(pe) == addr);
	return addr;
}

struct pagedir *pagedir_new(void) {
	struct pagedir *dir = page_alloc(1);
	memset(dir, 0, sizeof *dir);
	return dir;
}

void pagedir_free(struct pagedir *dir) {
	(void)dir;
	panic_unimplemented();
	/*
	// assumes all user pages are unique and can be freed
	struct pagetable_entry *pt;
	void *page;

	for (int i = 0; i < 1024; i++) {
		if (!dir->e[i].present) continue;

		pt = (void*)(dir->e[i].address << 11);

		for (int j = 0; j < 1024; j++) {
			if (!pt[j].present) continue;
			if (!pt[j].user)    continue;

			page = (void*)(pt[j].address << 11);
			page_free(page, 1);
		}
		page_free(pt, 1);
	}
	page_free(dir, 1);
	*/
}

/*
static struct pagetable_entry*
get_entry(struct pagedir *dir, const void __user *virt) {
	uint32_t pd_idx = ((uintptr_t)virt) >> 22;
	uint32_t pt_idx = ((uintptr_t)virt) >> 12 & 0x03FF;
	struct pagetable_entry *pagetable;

	if (!dir->e[pd_idx].present) return NULL;

	pagetable = (void*)(dir->e[pd_idx].address << 11);
	return &pagetable[pt_idx];
}
*/

void *pagedir_unmap(struct pagedir *dir, void __user *virt) {
	(void)dir; (void)virt;
	panic_unimplemented();
	/* unset the present bit
	void *phys = pagedir_virt2phys(dir, virt, false, false);
	struct pagetable_entry *page = get_entry(dir, virt);
	page->present = false;
	return phys;
	*/
}

void pagedir_map(struct pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable)
{
	pe_generic_t *pml4e, *pdpte, *pde, *pte;
	const union virt_addr v = {.full = virt};

	// TODO check if sign extension is valid

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
		pte->as_ptr = addr_validate(page_zalloc(1));
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
struct pagedir *pagedir_copy(const struct pagedir *orig) {
	/*
	struct pagedir *clone = page_alloc(1);
	struct pagetable_entry *orig_pt, *clone_pt;
	void *orig_page, *clone_page;

	for (int i = 0; i < 1024; i++) {
		clone->e[i] = orig->e[i];
		if (!orig->e[i].present) continue;

		orig_pt = (void*)(orig->e[i].address << 11);
		clone_pt = page_alloc(1);
		clone->e[i].address = (uintptr_t) clone_pt >> 11;

		for (int j = 0; j < 1024; j++) {
			clone_pt[j] = orig_pt[j];
			if (!orig_pt[j].present) continue;
			if (!orig_pt[j].user)    continue;
			// i could use .global?

			orig_page = (void*)(orig_pt[j].address << 11);
			clone_page = page_alloc(1);
			clone_pt[j].address = (uintptr_t) clone_page >> 11;

			memcpy(clone_page, orig_page, PAGE_SIZE);
		}
	}

	return clone;
	*/
	panic_unimplemented();
}

bool pagedir_iskern(struct pagedir *dir, const void __user *virt) {
	/*
	struct pagetable_entry *page = get_entry(dir, virt);
	return page && page->present && !page->user;
	*/
	panic_unimplemented();
}

void *pagedir_virt2phys(struct pagedir *dir, const void __user *virt,
                        bool user, bool writeable)
{
	/*
	struct pagetable_entry *page;
	uintptr_t phys;
	page = get_entry(dir, virt);
	if (!page || !page->present) return NULL;
	if (user && !page->user) return NULL;
	if (writeable && !page->writeable) return NULL;

	phys  = page->address << 11;
	phys |= ((uintptr_t)virt) & 0xFFF;
	return (void*)phys;
	*/
	panic_unimplemented();
}

void __user *pagedir_findfree(struct pagedir *dir, char __user *start, size_t len) {
	/*
	struct pagetable_entry *page;
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
	*/
	panic_unimplemented();
}

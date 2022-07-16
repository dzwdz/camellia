#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <shared/mem.h>
#include <stdint.h>

/* <heat> nitpick: I highly recommend you dont use bitfields for paging
 *        structures
 * <heat> you can't change them atomically and the way they're layed out
 *        in memory is implementation defined iirc
 */
struct pagetable_entry {
	uint32_t present   : 1;
	uint32_t writeable : 1;
	uint32_t user      : 1;
	uint32_t writethru : 1;
	uint32_t uncached  : 1;
	uint32_t dirty     : 1;
	uint32_t always0   : 1; // memory type thing?
	uint32_t global    : 1;
	uint32_t _unused   : 3;
	uint32_t address   : 21;
};

struct pagedir_entry {
	uint32_t present   : 1;
	uint32_t _writeable: 1; // don't use! not checked by multiple functions here
	uint32_t _user     : 1; // ^
	uint32_t writethru : 1;
	uint32_t uncached  : 1;
	uint32_t accessed  : 1;
	uint32_t always0   : 1;
	uint32_t large     : 1; // 4 MiB instead of 4 KiB
	uint32_t _unused   : 3;
	uint32_t address   : 21;
} __attribute__((packed));

struct pagedir {
	struct pagedir_entry e[1024];
} __attribute__((packed));


struct pagedir *pagedir_new(void) {
	struct pagedir *dir = page_alloc(1);
	for (int i = 0; i < 1024; i++)
		dir->e[i].present = 0;
	return dir;
}

void pagedir_free(struct pagedir *dir) {
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
}

static struct pagetable_entry*
get_entry(struct pagedir *dir, const void __user *virt) {
	uint32_t pd_idx = ((uintptr_t)virt) >> 22;
	uint32_t pt_idx = ((uintptr_t)virt) >> 12 & 0x03FF;
	struct pagetable_entry *pagetable;

	if (!dir->e[pd_idx].present) return NULL;

	pagetable = (void*)(dir->e[pd_idx].address << 11);
	return &pagetable[pt_idx];
}

void *pagedir_unmap(struct pagedir *dir, void __user *virt) {
	void *phys = pagedir_virt2phys(dir, virt, false, false);
	struct pagetable_entry *page = get_entry(dir, virt);
	page->present = false;
	return phys;
}

void pagedir_map(struct pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable)
{
	kprintf("in pagedir_map, dir 0x%x\n", dir);
	uintptr_t virt_cast = (uintptr_t) virt;
	uint32_t pd_idx = virt_cast >> 22;
	uint32_t pt_idx = virt_cast >> 12 & 0x03FF;
	struct pagetable_entry *pagetable;

	kprintf("pre-if, accessing 0x%x because 0x%x\n", &dir->e[pd_idx], pd_idx);
	if (dir->e[pd_idx].present) {
		kprintf("present already\n");
		pagetable = (void*) (dir->e[pd_idx].address << 11);
	} else {
		kprintf("allocing\n");
		pagetable = page_alloc(1);
		kprintf("alloc successful\n");
		for (int i = 0; i < 1024; i++)
			pagetable[i].present = 0;

		dir->e[pd_idx] = (struct pagedir_entry) {
			.present   = 1,
			._writeable= 1,
			._user     = 1,
			.writethru = 1,
			.uncached  = 0,
			.accessed  = 0,
			.always0   = 0,
			.large     = 0,
			._unused   = 0,
			.address   = (uintptr_t) pagetable >> 11
		};
	}

	pagetable[pt_idx] = (struct pagetable_entry) {
		.present   = 1,
		.writeable = writeable,
		.user      = user,
		.writethru = 1,
		.uncached  = 0,
		.dirty     = 0,
		.always0   = 0,
		.global    = 0,
		._unused   = 0,
		.address   = (uintptr_t) phys >> 11
	};
	kprintf("out pagedir_map\n");
}

extern void *pagedir_current;
void pagedir_switch(struct pagedir *dir) {
	pagedir_current = dir;
}

// creates a new pagedir with exact copies of the user pages
struct pagedir *pagedir_copy(const struct pagedir *orig) {
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
}

bool pagedir_iskern(struct pagedir *dir, const void __user *virt) {
	struct pagetable_entry *page = get_entry(dir, virt);
	return page && page->present && !page->user;
}

void *pagedir_virt2phys(struct pagedir *dir, const void __user *virt,
                        bool user, bool writeable)
{
	struct pagetable_entry *page;
	uintptr_t phys;
	page = get_entry(dir, virt);
	if (!page || !page->present) return NULL;
	if (user && !page->user) return NULL;
	if (writeable && !page->writeable) return NULL;

	phys  = page->address << 11;
	phys |= ((uintptr_t)virt) & 0xFFF;
	return (void*)phys;
}

void __user *pagedir_findfree(struct pagedir *dir, char __user *start, size_t len) {
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
}

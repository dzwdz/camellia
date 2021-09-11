#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/util.h>
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
	uint32_t writeable : 1;
	uint32_t user      : 1;
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

void pagedir_map(struct pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable)
{
	uintptr_t virt_cast = (uintptr_t) virt;
	uint32_t pd_idx = virt_cast >> 22;
	uint32_t pt_idx = virt_cast >> 12 & 0x03FF;
	struct pagetable_entry *pagetable;

	if (dir->e[pd_idx].present) {
		pagetable = (void*) (dir->e[pd_idx].address << 11);
	} else {
		pagetable = page_alloc(1);
		for (int i = 0; i < 1024; i++)
			pagetable[i].present = 0;

		dir->e[pd_idx] = (struct pagedir_entry) {
			.present   = 1,
			.writeable = 1,
			.user      = 1,
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
}

void pagedir_switch(struct pagedir *dir) {
	asm volatile("mov %0, %%cr3;" : : "r" (dir) : "memory");
}

// creates a new pagedir with exact copies of the user pages
struct pagedir *pagedir_copy(const struct pagedir *orig) {
	struct pagedir *clone = page_alloc(1);
	struct pagetable_entry *orig_pt, *clone_pt;
	void *orig_page, *clone_page;

	for (int i = 0; i < 1024; i++) {
		clone->e[i] = orig->e[i];
		if (!orig->e[i].present) continue;
		if (!orig->e[i].user)    continue; // not really needed

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

void *pagedir_virt2phys(struct pagedir *dir, const void __user *virt,
                        bool user, bool writeable)
{
	uintptr_t virt_cast = (uintptr_t) virt;
	uintptr_t phys;
	uint32_t pd_idx = virt_cast >> 22;
	uint32_t pt_idx = virt_cast >> 12 & 0x03FF;
	struct pagetable_entry *pagetable, page;

	/* DOESN'T CHECK PERMISSIONS ON PAGE DIRS, TODO
	 * while i don't currently see a reason to set permissions
	 * directly on page dirs, i might see one in the future.
	 * leaving this as-is would be a security bug */
	if (!dir->e[pd_idx].present) return 0;

	pagetable = (void*)(dir->e[pd_idx].address << 11);
	page      = pagetable[pt_idx];

	if (!page.present)                return 0;
	if (user      && !page.user)      return 0;
	if (writeable && !page.writeable) return 0;

	phys  = page.address << 11;
	phys |= virt_cast & 0xFFF;
	return (void*)phys;
}

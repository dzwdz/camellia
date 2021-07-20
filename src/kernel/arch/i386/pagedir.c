#include <kernel/arch/generic.h>
#include <kernel/mem.h>
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


struct pagedir *pagedir_new() {
	struct pagedir *dir = page_alloc(1);
	for (int i = 0; i < 1024; i++)
		dir->e[i].present = 0;
	return dir;
}

void pagedir_map(struct pagedir *dir, void *virt, void *phys,
                 bool user, bool writeable)
{
	uintptr_t virt_casted = (uintptr_t) virt;
	uint32_t pd_idx = virt_casted >> 22;
	uint32_t pt_idx = virt_casted >> 12 & 0x03FF;
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

void pagedir_use(struct pagedir *dir) {
	asm volatile("mov %0, %%cr3;" : : "r" (dir) : "memory");
}

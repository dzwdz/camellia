#pragma once
#include <shared/types.h>

/* <heat> nitpick: I highly recommend you dont use bitfields for paging
 *        structures
 * <heat> you can't change them atomically and the way they're layed out
 *        in memory is implementation defined iirc
 */
typedef union pe_generic_t {
	struct {
		uint64_t present   : 1;
		uint64_t writeable : 1;
		uint64_t user      : 1;
		uint64_t writethru : 1;

		uint64_t uncached  : 1;
		uint64_t accessed  : 1;
		uint64_t dirty     : 1;
		uint64_t large     : 1; // also, PAT

		uint64_t global    : 1; // CR4.PGE is disabled anyways
		uint64_t _unused2  : 2;
		uint64_t _unused3  : 1; // HLAT

		uint64_t address   : 40;

		uint64_t _unused4  : 7;
		uint64_t pke       : 4;
		uint64_t noexec    : 1;
	} __attribute__((packed));
	void *as_ptr;
} pe_generic_t; // pageentry_generic

struct pagedir { // on amd64 actually points to pml4. TODO more sensible type
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


/* amd64/32/paging.c */
extern pe_generic_t pml4_identity[512];
void pml4_identity_init(void); // called from amd64/32/boot.s

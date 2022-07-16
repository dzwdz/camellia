#include <stddef.h>
#include <stdint.h>

struct pml4e {
	uint64_t present   : 1;
	uint64_t writeable : 1;
	uint64_t user      : 1;
	uint64_t writethru : 1;

	uint64_t uncached  : 1;
	uint64_t accessed  : 1;
	uint64_t _unused1  : 1;
	uint64_t reserved  : 1; // always 0

	uint64_t _unused2  : 3;
	uint64_t _unused3  : 1; // HLAT thing

	uint64_t address   : 40;

	uint64_t _unused4  : 11;
	uint64_t noexec    : 1;
} __attribute__((packed));

struct pdpte { // page directory pointer table entry, 1gb page | 512 * pde
	uint64_t present   : 1;
	uint64_t writeable : 1;
	uint64_t user      : 1;
	uint64_t writethru : 1;

	uint64_t uncached  : 1;
	uint64_t accessed  : 1;
	uint64_t _unused1  : 1;
	uint64_t large     : 1; // 1gb page

	uint64_t _unused2  : 3;
	uint64_t _unused3  : 1; // HLAT

	uint64_t address   : 40;

	uint64_t _unused4  : 11;
	uint64_t noexec    : 1;
} __attribute__((packed));

struct pde { // page directory entry, 2mb page | 512 * pte
	uint64_t present   : 1;
	uint64_t writeable : 1;
	uint64_t user      : 1;
	uint64_t writethru : 1;

	uint64_t uncached  : 1;
	uint64_t accessed  : 1;
	uint64_t dirty     : 1; // only if large
	uint64_t large     : 1; // 2mb

	uint64_t global    : 1; // only if large ; TODO enable CR4.PGE
	uint64_t _unused2  : 2;
	uint64_t _unused3  : 1; // HLAT

	uint64_t address   : 40; // highest bit - PAT

	uint64_t _unused4  : 7;
	uint64_t pke       : 4;
	uint64_t noexec    : 1;
} __attribute__((packed));

struct pte { // page table entry, 4kb page
	uint64_t present   : 1;
	uint64_t writeable : 1;
	uint64_t user      : 1;
	uint64_t writethru : 1;

	uint64_t uncached  : 1;
	uint64_t accessed  : 1;
	uint64_t dirty     : 1;
	uint64_t pat       : 1;

	uint64_t global    : 1; // TODO enable CR4.PGE
	uint64_t _unused2  : 2;
	uint64_t _unused3  : 1; // HLAT

	uint64_t address   : 40;

	uint64_t _unused4  : 7;
	uint64_t pke       : 4;
	uint64_t noexec    : 1;
} __attribute__((packed));

__attribute__((aligned(4096)))
struct pml4e pml4_identity[512];

__attribute__((aligned(4096)))
struct pdpte pdpte_low[512]; // 0-512gb

__attribute__((aligned(4096)))
struct pde pde_low[512]; // 0-1gb


static void *memset32(void *s, int c, size_t n) {
	uint8_t *s2 = s;
	for (size_t i = 0; i < n; i++)
		s2[i] = c;
	return s;
}


void pml4_identity_init(void) {
	memset32(pml4_identity, 0, sizeof pml4_identity);
	memset32(pdpte_low, 0, sizeof pdpte_low);
	memset32(pde_low, 0, sizeof pde_low);

	pml4_identity[0] = (struct pml4e) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pdpte_low) >> 12,
	};

	pdpte_low[0] = (struct pdpte) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pde_low) >> 12,
	};

	for (int i = 0; i < 512; i++) {
		pde_low[i] = (struct pde) {
			.present = 1,
			.writeable = 1,
			.large = 1,
			.address = (i * 2 * 1024 * 1024) >> 12,
		};
	}
}

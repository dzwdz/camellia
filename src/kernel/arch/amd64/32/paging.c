#include <stddef.h>
#include <stdint.h>

struct page_generic {
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

__attribute__((aligned(4096)))
struct page_generic pml4_identity[512];

__attribute__((aligned(4096)))
struct page_generic pdpte_low[512]; // 0-512gb

__attribute__((aligned(4096)))
struct page_generic pde_low[512]; // 0-1gb


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

	pml4_identity[0] = (struct page_generic) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pdpte_low) >> 12,
	};

	pdpte_low[0] = (struct page_generic) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pde_low) >> 12,
	};

	for (int i = 0; i < 512; i++) {
		pde_low[i] = (struct page_generic) {
			.present = 1,
			.writeable = 1,
			.large = 1,
			.address = (i * 2 * 1024 * 1024) >> 12,
		};
	}
}

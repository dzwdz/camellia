#include <kernel/arch/amd64/32/util.h>
#include <kernel/arch/amd64/paging.h>

__attribute__((aligned(4096)))
pe_generic_t pml4_identity[512];

__attribute__((aligned(4096)))
static pe_generic_t pdpte_low[512]; // 0-512gb

__attribute__((aligned(4096)))
static pe_generic_t pde_low[512]; // 0-1gb

void pml4_identity_init(void) {
	memset32(pml4_identity, 0, sizeof pml4_identity);
	memset32(pdpte_low, 0, sizeof pdpte_low);
	memset32(pde_low, 0, sizeof pde_low);

	pml4_identity[0] = (pe_generic_t) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pdpte_low) >> 12,
	};

	pdpte_low[0] = (pe_generic_t) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pde_low) >> 12,
	};

	for (int i = 0; i < 512; i++) {
		pde_low[i] = (pe_generic_t) {
			.present = 1,
			.writeable = 1,
			.large = 1,
			.address = (i * 2 * 1024 * 1024) >> 12,
		};
	}
}

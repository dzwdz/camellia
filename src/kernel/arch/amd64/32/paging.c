#include <kernel/arch/amd64/32/util.h>
#include <kernel/arch/amd64/paging.h>

__attribute__((aligned(4096)))
pe_generic_t pml4_identity[512];

__attribute__((aligned(4096)))
static pe_generic_t pdpte_low[512]; // 0-512gb

__attribute__((aligned(4096)))
static pe_generic_t pde_low[4][512]; // 4 * 0-1gb

void pml4_identity_init(void) {
	memset32(pml4_identity, 0, sizeof pml4_identity);
	memset32(pdpte_low, 0, sizeof pdpte_low);
	memset32(pde_low, 0, sizeof pde_low);

	pml4_identity[0] = (pe_generic_t) {
		.present = 1,
		.writeable = 1,
		.address = ((uintptr_t)pdpte_low) >> 12,
	};

	for (int i = 0; i < 4; i++) {
		pdpte_low[i] = (pe_generic_t) {
			.present = 1,
			.writeable = 1,
			.address = ((uintptr_t)&pde_low[i]) >> 12,
		};

		for (int j = 0; j < 512; j++) {
			pde_low[i][j] = (pe_generic_t) {
				.present = 1,
				.writeable = 1,
				.large = 1,
				.address = (i * 512 + j) << 9,
			};
		}
	}
}

#include <kernel/arch/amd64/acpi.h>
#include <kernel/panic.h>
#include <shared/mem.h>
#include <stdint.h>

typedef struct {
	char sig[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t rev;
	uint32_t rsdt;
} __attribute__((packed)) RDSP;

typedef struct {
	char sig[4];
	uint32_t len;
	uint8_t rev;
	uint8_t checksum;
	char oemID[6];
	char oemTID[8];
	uint32_t oemRev;
	uint32_t creatorId;
	uint32_t creatorRev;
	char data[0];
} __attribute__((packed)) SDT;

/* https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf */
typedef struct {
	uint8_t where;
	uint8_t bitwidth;
	uint8_t bitoffset;
	uint8_t _reserved;
	uint64_t addr;
} __attribute__((packed)) GAS; /* Generic Address Structure */

typedef struct {
	SDT hdr;
	uint32_t _someid;
	GAS addr;
	/* ... */
} __attribute__((packed)) HPET;

void
acpi_parse(const void *rdsp_v)
{
	const RDSP *rdsp = rdsp_v;
	if (memcmp(rdsp->sig, "RSD PTR ", 8) != 0) {
		panic_invalid_state();
	}

	SDT *rsdt = (void*)(uintptr_t)rdsp->rsdt;
	if (memcmp(rsdt->sig, "RSDT ", 4) != 0) {
		panic_invalid_state();
	}

	uint32_t *sub = (void*)rsdt->data;
	int len = (rsdt->len - sizeof(SDT)) / 4;
	for (int i = 0; i < len; i++) {
		SDT *it = (void*)(uintptr_t)sub[i];
		if (memcmp(it->sig, "HPET", 4) == 0) {
			HPET *hpet = (void*)it;
			GAS *gas = &hpet->addr;
			if (gas->where == 0) { /* in memory */
				hpet_init((void*)gas->addr);
			}
		}
	}
}

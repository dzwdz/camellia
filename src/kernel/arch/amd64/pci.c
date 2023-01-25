#include <kernel/arch/amd64/driver/rtl8139.h>
#include <kernel/arch/amd64/interrupts.h>
#include <kernel/arch/amd64/pci.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/arch/generic.h>
#include <kernel/panic.h>

static const uint16_t CONFIG_ADDRESS = 0xCF8;
static const uint16_t CONFIG_DATA = 0xCFC;


uint8_t pcicfg_r8(uint32_t bdf, uint32_t offset) {
	return pcicfg_r32(bdf, offset) >> ((offset & 3) * 8);
}

uint16_t pcicfg_r16(uint32_t bdf, uint32_t offset) {
	return pcicfg_r32(bdf, offset) >> ((offset & 2) * 8);
}

uint32_t pcicfg_r32(uint32_t bdf, uint32_t offset) {
	port_out32(CONFIG_ADDRESS, 0x80000000 | bdf | (offset & ~3));
	return port_in32(CONFIG_DATA);
}

void pcicfg_w32(uint32_t bdf, uint32_t offset, uint32_t value) {
	port_out32(CONFIG_ADDRESS, 0x80000000 | bdf | (offset & ~3));
	port_out32(CONFIG_DATA, value);
}


uint16_t pcicfg_iobase(uint32_t bdf) {
	/* cuts corners, assumes header type 0, etc */
	uint32_t bar = pcicfg_r32(bdf, 0x10);
	if (!(bar & 1))
		panic_unimplemented(); /* not an io bar */
	return bar & ~3;
}

static uint32_t bdf_of(uint32_t bus, uint32_t device, uint32_t func) {
	return (bus << 16) | (device << 11) | (func << 8);
}

static void scan_bus(uint32_t bus) {
	for (int slot = 0; slot < 32; slot++) {
		int fn_amt = 1;
		for (int fn = 0; fn < fn_amt; fn++) {
			uint32_t bdf = bdf_of(bus, slot, fn);
			uint32_t id = pcicfg_r32(bdf, 0);
			if (id == 0xFFFFFFFF) break;
			kprintf("pci %02x:%02x.%x\t%x\n", bus, slot, fn, id);

			uint8_t hdr_type = pcicfg_r8(bdf, 0xE);
			if (hdr_type & 0x80) {
				fn_amt = 8;
				hdr_type &= ~0x80;
			}
			if (hdr_type != 0) {
				kprintf("pci: skipping unknown header %x\n", hdr_type);
				continue;
			}

			if (id == 0x813910ec) {
				pcicfg_w32(bdf, 0x3C, IRQ_RTL8139);
				rtl8139_init(bdf);
			}
		}
	}
}

void pci_init(void) {
	scan_bus(0);
	// TODO multiple host controllers
}

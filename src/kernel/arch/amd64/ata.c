#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/panic.h>
#include <stdbool.h>

static struct {
	enum {
		DEV_UNKNOWN,
		DEV_PATA,
		DEV_PATAPI,
	} type;
	uint32_t sectors;
} ata_drives[4];

enum {
	DATA   = 0,
	FEAT   = 1,
	SCNT   = 2,
	LBAlo  = 3,
	LBAmid = 4,
	LBAhi  = 5,
	DRV    = 6,
	CMD    = 7,
	STATUS = 7,

	CTRL   = 0x206,
}; // offsets

// get I/O port base for drive
static uint16_t ata_iobase(int drive) {
	bool secondary = drive&2;
	return secondary ? 0x170 : 0x1F0;
}

static void ata_400ns(void) {
	uint16_t base = ata_iobase(0); // doesn't matter
	for (int i = 0; i < 4; i++)
		port_in8(base + STATUS);
}

static void ata_driveselect(int drive, int lba) {
	uint8_t v = 0xE0;
	if (drive&1) // slave?
		v |= 0x10; // set drive number bit
	v |= (lba >> 24) & 0xf;
	port_out8(ata_iobase(drive) + DRV, v);
}

static void ata_softreset(int drive) {
	uint16_t iobase = ata_iobase(drive);
	port_out8(iobase + CTRL, 4);
	port_out8(iobase + CTRL, 0);
	ata_400ns();

	uint16_t timeout = 10000;
	while (--timeout) { // TODO separate polling function
		uint8_t v = port_in8(iobase + STATUS);
		if (v & 0x80) continue; // still BSY, continue
		if (v & 0x40) break; // RDY, break
		// TODO check for ERR
	}
}

static void ata_detecttype(int drive) {
	ata_softreset(drive);
	ata_driveselect(drive, 0);
	ata_400ns();
	switch (port_in8(ata_iobase(drive) + LBAmid)) {
		case 0:
			ata_drives[drive].type = DEV_PATA;
			break;
		case 0x14:
			ata_drives[drive].type = DEV_PATAPI;
			break;
		default:
			ata_drives[drive].type = DEV_UNKNOWN;
			break;
	}
}

static bool ata_identify(int drive) {
	uint16_t iobase = ata_iobase(drive);
	uint16_t data[256];
	uint8_t v;

	ata_driveselect(drive, 0);
	for (int i = 2; i < 6; i++)
		port_out8(iobase + i, 0);
	switch (ata_drives[drive].type) {
		case DEV_PATA:
			port_out8(iobase + CMD, 0xEC); // IDENTIFY
			break;
		case DEV_PATAPI:
			port_out8(iobase + CMD, 0xA1); // IDENTIFY PACKET DEVICE
			break;
		default: panic_invalid_state();
	}

	v = port_in8(iobase + STATUS);
	if (v == 0) return false; // nonexistent drive
	while (port_in8(iobase + STATUS) & 0x80);

	/* pool until bit 3 (DRQ) or 0 (ERR) is set */
	while (!((v = port_in8(iobase + STATUS) & 0x9)));
	if (v & 1) return false; /* ERR was set, bail */

	for (int i = 0; i < 256; i++)
		data[i] = port_in16(iobase);
	ata_drives[drive].sectors = data[60] | (data[61] << 16);
	return true;
}

void ata_init(void) {
	for (int i = 0; i < 4; i++) {
		ata_detecttype(i);
		if (ata_drives[i].type == DEV_PATA)
			ata_identify(i);
	}
}

bool ata_available(int drive) {
	return ata_drives[drive].type != DEV_UNKNOWN;
}

size_t ata_size(int drive) {
	return ata_drives[drive].sectors * ATA_SECTOR;
}

int ata_read(int drive, uint32_t lba, void *buf) {
	if (ata_drives[drive].type != DEV_PATA)
		panic_unimplemented();
	int iobase = ata_iobase(drive);

	ata_driveselect(drive, lba);
	port_out8(iobase + FEAT, 0); // supposedly pointless
	port_out8(iobase + SCNT, 1); // sector count
	port_out8(iobase + LBAlo, lba);
	port_out8(iobase + LBAmid, lba >> 8);
	port_out8(iobase + LBAhi, lba >> 16);
	port_out8(iobase + CMD, 0x20); // READ SECTORS

	for (;;) { // TODO separate polling function
		uint8_t v = port_in8(iobase + STATUS);
		if (v & 0x80) continue; // still BSY, continue
		if (v & 0x40) break; // RDY, break
		// TODO check for ERR
	}

	uint16_t *b = buf;
	for (int i = 0; i < 256; i++)
		b[i] = port_in16(iobase);

	return 512;
}

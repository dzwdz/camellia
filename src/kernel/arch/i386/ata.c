#include <kernel/arch/i386/ata.h>
#include <kernel/arch/i386/port_io.h>
#include <stdbool.h>

#include <kernel/arch/io.h>

static struct {
	enum {
		DEV_UNKNOWN,
		DEV_PATA,
		DEV_PATAPI,
	} type;
	uint32_t sectors;
} ata_drives[4];

enum {
	LBAlo  = 3,
	LBAmid = 4,
	LBAhi  = 5,
	DRV    = 6,
	CMD    = 7,
	STATUS = 7,

	/* note: the OSDev wiki uses a different base port for the control port
	 *       however i can just use this offset and stuff will just work tm */
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

static void ata_driveselect(int drive, int block) {
	uint8_t v = 0xE0;
	if (drive&1) // slave?
		v |= 0x10; // set drive number bit
	// TODO account for block
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
			return true;
		default:
			ata_drives[drive].type = DEV_UNKNOWN;
			return false;
	}
}

static bool ata_identify(int drive) {
	uint16_t iobase = ata_iobase(drive);
	uint16_t data[256];
	uint8_t v;

	// TODO test for float

	ata_driveselect(drive, 0);
	for (int i = 2; i < 6; i++)
		port_out8(iobase + i, 0);
	port_out8(iobase + CMD, 0xEC); // IDENTIFY

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
	tty_const("\n");
	for (int i = 0; i < 4; i++) {
		tty_const("probing drive ");
		_tty_var(i);
		ata_detecttype(i);
		_tty_var(ata_drives[i].type);
		// if (ata_identify(i)) {
		// 	tty_const(" - ");
		// 	_tty_var(ata_drives[i].sectors);
		// 	tty_const(" sectors (512b)");
		// }
		tty_const("\n");
	}
}

#include <kernel/arch/i386/ata.h>
#include <kernel/arch/i386/port_io.h>
#include <stdbool.h>

#include <kernel/arch/io.h>

enum {
	LBAlo  = 3,
	LBAmid = 4,
	LBAhi  = 5,
	DRV    = 6,
	CMD    = 7,
	STATUS = 7,
}; // offsets

// get I/O port base for drive
static uint16_t ata_iobase(int drive) {
	bool secondary = drive&2;
	return secondary ? 0x170 : 0x1F0;
}

static void ata_driveselect(int drive, int block) {
	uint8_t v = 0xE0;
	if (drive&1) // slave?
		v |= 0x10; // set drive number bit
	// TODO account for block
	port_out8(ata_iobase(drive) + DRV, v);
}

static bool ata_identify(int drive) {
	uint16_t iobase = ata_iobase(drive);
	uint8_t v;

	ata_driveselect(drive, 0);
	for (int i = 2; i < 6; i++)
		port_out8(iobase + i, 0);
	port_out8(iobase + CMD, 0xEC); // IDENTIFY

	v = port_in8(iobase + STATUS);
	if (v == 0) return false; // nonexistent drive
	while (port_in8(iobase + STATUS) & 0x80);

	/* check for uncomformant devices, quit early */
	if (port_in8(iobase + LBAmid) || port_in8(iobase + LBAhi)) {
		// TODO atapi
		return true;
	}
	/* pool until bit 3 (DRQ) or 0 (ERR) is set */
	while (!((v = port_in8(iobase + STATUS) & 0x9)));
	if (v & 1) return false; /* ERR was set, bail */

	// now I can read 512 bytes of data, TODO
	return true;
}

void ata_init(void) {
	tty_const("\n");
	for (int i = 0; i < 4; i++) {
		tty_const("probing drive ");
		_tty_var(i);
		if (ata_identify(i))
			tty_const(" - exists");
		tty_const("\n");
	}
}

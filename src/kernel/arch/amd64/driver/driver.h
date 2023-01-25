#pragma once
#include <kernel/types.h>
#include <stddef.h>
#include <stdint.h>

struct GfxInfo {
	char *b;
	uint32_t width, height;
	uint32_t pitch; /* width in bytes of a single scanline */
	size_t size;
	uint8_t bpp;
};

void pata_init(void);
void ps2_init(void);
void rtl8139_init(uint32_t bdf);
void vfs_root_init(void);
void video_init(GfxInfo);

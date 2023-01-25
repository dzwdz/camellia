#pragma once
#include <stdint.h>

struct GfxInfo {
	char *b;
	uint32_t width, height;
	uint32_t pitch; /* width in bytes of a single scanline */
	size_t size;
	uint8_t bpp;
};

void video_init(GfxInfo);

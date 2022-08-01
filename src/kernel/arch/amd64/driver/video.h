#pragma once
#include <stdint.h>

struct fb_info {
	char *b;
	uint32_t width, height;
	uint32_t pitch; /* width in bytes of a single scanline */
	uint8_t bpp;
};

void video_init(struct fb_info);

#pragma once
#include <camellia/types.h>
#include <stddef.h>
#include <stdint.h>

struct framebuf {
	size_t len, width, height, pitch;
	uint8_t bpp;
	char *b;

	hid_t fd;
};

struct rect { uint32_t x1, y1, x2, y2; };
void dirty_reset(struct rect *d);
void dirty_mark(struct rect *d, uint32_t x, uint32_t y);
void dirty_flush(struct rect *d, struct framebuf *fb);

int fb_setup(struct framebuf *fb, const char *base);
int fb_anon(struct framebuf *fb, size_t w, size_t h);
uint32_t *fb_pixel(struct framebuf *fb, uint32_t x, uint32_t y);
void fb_cpy(
	struct framebuf *dest, const struct framebuf *src,
	size_t xd, size_t yd, size_t xs, size_t ys, size_t w, size_t h);

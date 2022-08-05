#pragma once
#include <camellia/types.h>
#include <stdint.h>
#include <stdio.h>

#define eprintf(fmt, ...) fprintf(stderr, "vterm: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)


struct psf {
    uint32_t magic;
    uint32_t version;
    uint32_t glyph_offset;
    uint32_t flags;
    uint32_t glyph_amt;
    uint32_t glyph_size;
    uint32_t h;
    uint32_t w;
} __attribute__((packed));
extern struct psf font;
extern void *font_data;
void font_load(const char *path);
void font_blit(uint32_t glyph, int x, int y);


struct framebuf {
	size_t len, width, height, pitch;
	uint8_t bpp;
	char *b;
};
extern struct framebuf fb;
extern handle_t fb_fd;

struct rect {
	uint32_t x1, y1, x2, y2;
};
extern struct rect dirty;
void dirty_mark(uint32_t x, uint32_t y);
void flush(void);
void scroll(void);

struct point {uint32_t x, y;};
extern struct point cursor;
void in_char(char c);

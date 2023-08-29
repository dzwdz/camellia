#pragma once
#include <camellia/types.h>
#include <stdint.h>
#include <stdio.h>
#include <draw.h>

struct psf1 {
    uint16_t magic;
	uint8_t mode;
	uint8_t h;
} __attribute__((packed));
struct psf2 {
    uint32_t magic;
    uint32_t version;
    uint32_t glyph_offset;
    uint32_t flags;
    uint32_t glyph_amt;
    uint32_t glyph_size;
    uint32_t h;
    uint32_t w;
} __attribute__((packed));
extern struct psf2 font;
extern void *font_data;
void font_load(const char *path);
void font_blit(uint32_t glyph, int x, int y);

extern struct framebuf fb;

extern struct rect dirty;
void vdirty_mark(uint32_t x, uint32_t y);
void flush(void);
void scroll(void);

struct point {uint32_t x, y;};
extern struct point cursor;
void in_char(char c);

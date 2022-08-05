#include "vterm.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct psf font;
void *font_data;

void font_load(const char *path) {
	FILE *f = fopen(path, "r");
	if (!f) {
		eprintf("couldn't open font \"%s\"", path);
		exit(1);
	}

	void *buf;
	long buflen;

	fseek(f, 0, SEEK_END);
	buflen = ftell(f);
	if (buflen < 0) {
		eprintf("can't get the size of \"%s\"", path);
		exit(1);
	}

	buf = malloc(buflen);
	if (!buf) {
		eprintf("out of memory");
		exit(1);
	}

	fseek(f, 0, SEEK_SET);
	fread(buf, 1, buflen, f);
	if (ferror(f)) {
		eprintf("error reading file");
		exit(1);
	}
	fclose(f);

	if (memcmp(buf, "\x72\xb5\x4a\x86", 4)) {
		eprintf("invalid psf header");
		exit(1);
	}
	memcpy(&font, buf, sizeof font);
	font_data = buf + font.glyph_offset;
}

void font_blit(uint32_t glyph, int x, int y) {
	if (glyph >= font.glyph_amt) glyph = 0;

	dirty_mark(x, y);

	char *bitmap = font_data + font.glyph_size * glyph;
	for (size_t i = 0; i < font.w; i++) {
		for (size_t j = 0; j < font.h; j++) {
			size_t idx = j * font.w + i;
			char byte = bitmap[idx / 8];
			byte >>= (7-(idx&7));
			byte &= 1;
			*((uint32_t*)&fb.b[fb.pitch * (y * font.h + j) + 4 * (x * font.w + i)]) = byte * 0xB0B0B0;
		}
	}
}

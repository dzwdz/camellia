#include "vterm.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct psf2 font;
void *font_data;

void font_load(const char *path) {
	FILE *f = fopen(path, "r");
	if (!f) {
		err(1, "couldn't open font \"%s\"", path);
	}

	void *buf;
	long buflen;

	fseek(f, 0, SEEK_END);
	buflen = ftell(f);
	if (buflen < 0) {
		errx(1, "can't get the size of \"%s\"", path);
	}

	buf = malloc(buflen);
	if (!buf) {
		err(1, "out of memory");
	}

	fseek(f, 0, SEEK_SET);
	fread(buf, 1, buflen, f);
	if (ferror(f)) {
		err(1, "error reading font");
	}
	fclose(f);

	if (!memcmp(buf, "\x72\xb5\x4a\x86", 4)) {
		memcpy(&font, buf, sizeof font);
		font_data = buf + font.glyph_offset;
	} else if (!memcmp(buf, "\x36\x04", 2)) {
		struct psf1 *hdr = buf;
		font = (struct psf2){
			.glyph_amt = 256,
			.glyph_size = hdr->h,
			.h = hdr->h,
			.w = 8,
		};
		font_data = buf + 4;
	} else {
		errx(1, "invalid psf header");
	}
}

void font_blit(uint32_t glyph, int x, int y) {
	if (glyph >= font.glyph_amt) glyph = 0;
	if (x < 0 || (x+1) * font.w >= fb.width ||
		y < 0 || (y+1) * font.h >= fb.height)
	{
		return;
	}

	dirty_mark(&dirty, x * font.w, y * font.h);
	dirty_mark(&dirty, (x+1) * font.w - 1, (y+1) * font.h - 1);

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
	return;
}

#include "driver.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define eprintf(fmt, ...) fprintf(stderr, "ansiterm: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

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

static handle_t fb_fd;

static struct {int x, y;} cursor = {0};

static size_t dirty_low = ~0, dirty_high = 0;

// TODO don't hardcode size
static const size_t fb_len = 640 * 480 * 4;
static const size_t fb_width = 640;
static const size_t fb_height = 480;
static const size_t fb_pitch = 640 * 4;
static char *fb;

static struct psf font;
static void *font_data;


static void flush(void) {
	if (dirty_high == 0) return;

	if (dirty_high > fb_len) // shouldn't happen
		dirty_high = fb_len;

	// still not optimal, copies 80 characters instead of 1
	while (dirty_low < dirty_high)
		dirty_low += _syscall_write(fb_fd, fb + dirty_low, dirty_high - dirty_low, dirty_low, 0);

	dirty_low = ~0;
	dirty_high = 0;
}

static void scroll(void) {
	// TODO memmove. this is UD
	size_t row_len = fb_pitch * font.h;
	memcpy(fb, fb + row_len, fb_len - row_len);
	memset(fb + fb_len - row_len, 0, row_len);
	cursor.y--;
	dirty_low = 0; dirty_high = ~0;
}

static void font_blit(size_t glyph, int x, int y) {
	if (glyph >= font.glyph_amt) glyph = 0;

	size_t low  = fb_pitch * ( y    * font.h) + 4 * ( x    * font.w);
	size_t high = fb_pitch * ((y+1) * font.h) + 4 * ((x+1) * font.w) + 3;
	if (dirty_low > low)
		dirty_low = low;
	if (dirty_high < high)
		dirty_high = high;

	char *bitmap = font_data + font.glyph_size * glyph;
	for (size_t i = 0; i < font.w; i++) {
		for (size_t j = 0; j < font.h; j++) {
			size_t idx = j * font.w + i;
			char byte = bitmap[idx / 8];
			byte >>= (7-(idx&7));
			byte &= 1;
			*((uint32_t*)&fb[fb_pitch * (y * font.h + j) + 4 * (x * font.w + i)]) = byte * 0xB0B0B0;
		}
	}
}

static void in_char(char c) {
	switch (c) {
		case '\n':
			cursor.x = 0;
			cursor.y++;
			break;
		case '\b':
			if (--cursor.x < 0) cursor.x = 0;
			break;
		default:
			font_blit(c, cursor.x, cursor.y);
			cursor.x++;
	}

	if (cursor.x * font.w >= fb_width) {
		cursor.x = 0;
		cursor.y++;
	}
	while (cursor.y * font.h >= fb_height) scroll();
}

static void font_load(void) {
	FILE *f = fopen("/init/font.psf", "r");
	if (!f) {
		eprintf("couldn't open font file");
		exit(1);
	}

	const size_t cap = 8 * 1024; // TODO get file size
	void *buf = malloc(cap);
	if (!buf) {
		eprintf("out of memory");
		exit(1);
	}
	fread(buf, 1, cap, f);
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

void ansiterm_drv(void) {
	fb = malloc(fb_len);
	fb_fd = _syscall_open("/kdev/video/b", 13, 0);

	font_load();

	cursor.x = 0;
	cursor.y = 0;
	flush();

	static char buf[512];
	struct fs_wait_response res;
	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				// TODO check path
				_syscall_fs_respond(NULL, 0, 0);
				break;

			case VFSOP_WRITE:
				if (res.flags) {
					_syscall_fs_respond(NULL, -1, 0);
				} else {
					for (size_t i = 0; i < res.len; i++)
						in_char(buf[i]);
					flush();
					_syscall_fs_respond(NULL, res.len, 0);
				}
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}

	exit(1);
}

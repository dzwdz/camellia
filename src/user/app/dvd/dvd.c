#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define eprintf(fmt, ...) fprintf(stderr, "vterm: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

struct framebuf {
	size_t len, width, height, pitch;
	uint8_t bpp;
	char *b;
};

struct rect {
	uint32_t x1, y1, x2, y2;
};

struct framebuf fb;
handle_t fb_fd;
struct rect dirty;


void dirty_mark(uint32_t x, uint32_t y) {
	if (dirty.x1 > x) dirty.x1 = x;
	if (dirty.x2 < x) dirty.x2 = x;
	if (dirty.y1 > y) dirty.y1 = y;
	if (dirty.y2 < y) dirty.y2 = y;
}

static void flush_combined(struct rect pix) {
	size_t low  = fb.pitch * pix.y1 + 4 * pix.x1;
	size_t high = fb.pitch * pix.y2 + 4 * pix.y2 + 4;

	_syscall_write(fb_fd, fb.b + low, high - low, low, 0);
}

static void flush_split(struct rect pix) {
	static uint64_t execbuf[EXECBUF_MAX_LEN / sizeof(uint64_t)];
	if (7 * (pix.y2 - pix.y1) * sizeof(uint64_t) >= sizeof execbuf) {
		flush_combined(pix);
		return;
	}

	size_t epos = 0;
	for (uint32_t y = pix.y1; y < pix.y2; y++) {
		size_t low  = fb.pitch * y + 4 * pix.x1;
		size_t high = fb.pitch * y + 4 * pix.x2 + 4;

		execbuf[epos++] = EXECBUF_SYSCALL;
		execbuf[epos++] = _SYSCALL_WRITE;
		execbuf[epos++] = fb_fd;
		execbuf[epos++] = fb.b + low;
		execbuf[epos++] = high - low;
		execbuf[epos++] = low;
		execbuf[epos++] = 0;
	}
	_syscall_execbuf(execbuf, epos * sizeof(uint64_t));
}

void flush(void) {
	if (~dirty.x1 == 0) return;

	if (dirty.x2 >= fb.width)  dirty.x2 = fb.width - 1;
	if (dirty.y2 >= fb.height) dirty.y2 = fb.height - 1;

	/* the threshold is mostly arbitrary, wasn't based on any real benchmarks */
	if (dirty.x2 - dirty.x1 > fb.width - 600) flush_combined(dirty);
	else flush_split(dirty);

	dirty.x1 = ~0; dirty.y1 = ~0;
	dirty.x2 =  0; dirty.y2 =  0;
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col) {
	for (uint32_t i = 0; i < w; i++) {
		for (uint32_t j = 0; j < h; j++) {
			*((uint32_t*)(fb.b + fb.pitch * (y+j) + 4 * (x+i))) = col;
		}
	}
	if (dirty.x1 > x) dirty.x1 = x;
	if (dirty.y1 > y) dirty.y1 = y;
	if (dirty.x2 < x + w) dirty.x2 = x + w;
	if (dirty.y2 < y + h) dirty.y2 = y + h;
}

void fb_setup(void) {
	#define BASEPATH "/kdev/video/"
	char path[64], *spec;
	size_t pos;
	FILE *f;

	f = fopen(BASEPATH, "r");
	if (!f) {
		eprintf("couldn't open %s", BASEPATH);
		exit(1);
	}

	pos = strlen(BASEPATH);
	memcpy(path, BASEPATH, pos);
	spec = path + pos;
	fread(spec, 1, sizeof(path) - pos, f);
	/* assumes the read went correctly */
	fclose(f);

	fb_fd = _syscall_open(path, strlen(path), 0);
	if (fb_fd < 0) {
		eprintf("failed to open %s", path);
		exit(1);
	}

	fb.width  = strtol(spec, &spec, 0);
	if (*spec++ != 'x') { eprintf("bad filename format"); exit(1); }
	fb.height = strtol(spec, &spec, 0);
	if (*spec++ != 'x') { eprintf("bad filename format"); exit(1); }
	fb.bpp = strtol(spec, &spec, 0);
	fb.len = _syscall_getsize(fb_fd);
	fb.pitch = fb.len / fb.height;
	fb.b = malloc(fb.len);

	if (fb.bpp != 32) {
		eprintf("unsupported format %ux%ux%u", fb.width, fb.height, fb.bpp);
		exit(1);
	}
}

int main(void) {
	fb_setup();
	int dx = 2, dy = 2, x = 100, y = 100, w = 150, h = 70;
	uint32_t col = 0x800000;

	for (;;) {
		if (x + dx < 0 || x + dx + w >= fb.width ) dx *= -1;
		if (y + dy < 0 || y + dy + h >= fb.height) dy *= -1;
		x += dx;
		y += dy;
		draw_rect(x, y, w, h, col++);
		flush();
		_syscall_sleep(1000 / 60);
	}

	return 1;
}

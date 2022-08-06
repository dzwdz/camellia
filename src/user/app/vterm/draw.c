#include "vterm.h"
#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <string.h>

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

	struct rect pix;
	pix.x1 = dirty.x1 * font.w;
	pix.x2 = dirty.x2 * font.w + font.w - 1;
	pix.y1 = dirty.y1 * font.h;
	pix.y2 = dirty.y2 * font.h + font.h - 1;
	if (pix.x2 >= fb.width)  pix.x2 = fb.width - 1;
	if (pix.y2 >= fb.height) pix.y2 = fb.height - 1;

	/* the threshold is mostly arbitrary, wasn't based on any real benchmarks */
	if (pix.x2 - pix.x1 > fb.width - 600) flush_combined(pix);
	else flush_split(pix);

	dirty.x1 = ~0; dirty.y1 = ~0;
	dirty.x2 =  0; dirty.y2 =  0;
}

void scroll(void) {
	// TODO memmove. this is UD
	size_t row_len = fb.pitch * font.h;
	memcpy(fb.b, fb.b + row_len, fb.len - row_len);
	memset(fb.b + fb.len - row_len, 0, row_len);
	cursor.y--;

	dirty.x1 =  0; dirty.y1 =  0;
	dirty.x2 = ~0; dirty.y2 = ~0;
}

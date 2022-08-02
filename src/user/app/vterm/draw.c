#include "vterm.h"
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

void flush(void) {
	size_t low, high;
	if (~dirty.x1 == 0) return;

	// still not optimal, copies 80 characters instead of 1
	low  = fb.pitch * ( dirty.y1    * font.h) + 4 * ( dirty.x1    * font.w);
	high = fb.pitch * ((dirty.y2+1) * font.h) + 4 * ((dirty.x2+1) * font.w) + 3;

	if (~dirty.y2 == 0) high = fb.len;
	if (high > fb.len)  high = fb.len;

	while (low < high)
		low += _syscall_write(fb_fd, fb.b + low, high - low, low, 0);

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

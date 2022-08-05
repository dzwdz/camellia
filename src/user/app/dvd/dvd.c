#include <camellia/syscalls.h>
#include <stdlib.h>

struct framebuf {
	size_t len, width, height, pitch;
	char *b;
};
struct framebuf fb;
handle_t fb_fd;

struct rect {
	uint32_t x1, y1, x2, y2;
};
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

	// not optimal, on larger display widths it'd be more efficient to split this up into multiple writes
	low  = fb.pitch * (dirty.y1  ) + 4 * (dirty.x1  );
	high = fb.pitch * (dirty.y2+1) + 4 * (dirty.x2+1) + 3;

	if (~dirty.y2 == 0) high = fb.len;
	if (high > fb.len)  high = fb.len;

	_syscall_write(fb_fd, fb.b + low, high - low, low, 0);

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

int main(void) {
	fb_fd = _syscall_open("/kdev/video/b", 13, 0);
	// TODO don't hardcode framebuffer size
	fb.len = 640 * 480 * 4;
	fb.width = 640;
	fb.height = 480;
	fb.pitch = 640 * 4;
	fb.b = malloc(fb.len);

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

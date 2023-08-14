#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <draw.h>

#define eprintf(fmt, ...) fprintf(stderr, "vterm: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

struct framebuf fb;
struct rect dirty;

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col) {
	for (uint32_t i = 0; i < w; i++) {
		for (uint32_t j = 0; j < h; j++) {
			*((uint32_t*)(fb.b + fb.pitch * (y+j) + 4 * (x+i))) = col;
		}
	}
	dirty_mark(&dirty, x, y);
	dirty_mark(&dirty, x + w, y + h);
}

int main(void) {
	if (fb_setup(&fb, "/kdev/video/") < 0) {
		eprintf("fb_setup error");
		return 1;
	}
	int dx = 2, dy = 2, x = 100, y = 100, w = 150, h = 70;
	uint32_t col = 0x800000;

	for (;;) {
		if (x + dx < 0 || (size_t)(x + dx + w) >= fb.width)  dx *= -1;
		if (y + dy < 0 || (size_t)(y + dy + h) >= fb.height) dy *= -1;
		x += dx;
		y += dy;
		draw_rect(x, y, w, h, col++);
		dirty_flush(&dirty, &fb);
		_sys_sleep(1000 / 60);
	}

	return 1;
}

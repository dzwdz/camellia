#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <shared/container/ring.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/draw/draw.h>

#define MOUSE_SIZE 10

#define eprintf(fmt, ...) fprintf(stderr, "drawmouse: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

struct framebuf fb, lastbehind;
struct rect dirty;

static uint8_t r_buf[64];
static ring_t r = {(void*)r_buf, sizeof r_buf, 0, 0};
static struct {uint32_t x, y;} cursor, lastcur;


static void draw_mouse(void) {
	static bool drawn = false;
	if (drawn)
		fb_cpy(&fb, &lastbehind, lastcur.x, lastcur.y, 0, 0, MOUSE_SIZE, MOUSE_SIZE);
	drawn = true;
	dirty_mark(&dirty, lastcur.x, lastcur.y);
	dirty_mark(&dirty, lastcur.x + MOUSE_SIZE, lastcur.y + MOUSE_SIZE);
	fb_cpy(&lastbehind, &fb, 0, 0, cursor.x, cursor.y, MOUSE_SIZE, MOUSE_SIZE);
	for (int i = 0; i < MOUSE_SIZE; i++) {
		for (int j = 0; j < MOUSE_SIZE - i; j++) {
			uint32_t *px = fb_pixel(&fb, cursor.x + i, cursor.y + j);
			if (px) {
				*px ^= 0x808080;
				dirty_mark(&dirty, cursor.x + i, cursor.y + j);
			}
		}
	}
	lastcur = cursor;
}


struct packet {
	uint8_t left     : 1;
	uint8_t right    : 1;
	uint8_t middle   : 1;
	uint8_t _useless : 5;
	int8_t dx, dy;
} __attribute__((packed));

int main(void) {
	char buf[64];
	handle_t fd = _syscall_open("/kdev/ps2/mouse", 15, OPEN_READ);
	if (fd < 0) {
		eprintf("couldn't open mouse");
		return 1;
	}

	if (fb_setup(&fb, "/kdev/video/") < 0) {
		eprintf("fb_setup error");
		return 1;
	}
	fb_anon(&lastbehind, MOUSE_SIZE, MOUSE_SIZE);


	for (;;) {
		int len = _syscall_read(fd, buf, sizeof buf, 0);
		if (len == 0) break;
		ring_put(&r, buf, len);
		while (ring_used(&r) >= 3) {
			struct packet p;
			ring_get(&r, &p, sizeof p);
			p.dy *= -1;
			// TODO check mouse click
			if (-p.dx > (int)cursor.x) p.dx = -cursor.x;
			if (-p.dy > (int)cursor.y) p.dy = -cursor.y;
			cursor.x += p.dx;
			cursor.y += p.dy;
			if (cursor.x >= fb.width)  cursor.x = fb.width - 1;
			if (cursor.y >= fb.height) cursor.y = fb.height - 1;
			draw_mouse();
			if (p.left && p.right) return 0;
		}
		dirty_flush(&dirty, &fb);
	}
	return 0;
}

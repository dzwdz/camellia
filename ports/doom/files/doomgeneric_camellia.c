#include <camellia.h>
#include <camellia/syscalls.h>
#include <shared/ring.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <draw.h>
#include <thread.h>

#include "doomgeneric.h"
#include "doomkeys.h"

static struct framebuf fb;
static hid_t kb;

static const char keymap_lower[] = {
	'\0', '\0', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\r', '\0', 'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  '\0', '\'', '`',  '\0', '\\', 'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',  '\0', '*',  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
};

static char kb_backbuf[256];
static ring_t kb_backlog = {kb_backbuf, sizeof kb_backbuf, 0, 0};

static void kb_thread(void *arg) {
	(void)arg;
	char buf[sizeof kb_backbuf];
	for (;;) {
		int ret = _sys_read(kb, buf, sizeof buf, 0);
		if (ret <= 0) break;
		ring_put(&kb_backlog, buf, ret);
	}
}

void DG_Init(void) {
	if (fb_setup(&fb, "/dev/video/") < 0) {
		puts("DG_Init: fb_setup error");
		abort();
	}
	kb = camellia_open("/dev/ps2/kb", OPEN_READ);
	if (kb < 0) {
		puts("DG_Init: can't open keyboard");
		abort();
	}
	thread_create(0, kb_thread, NULL);
}

void DG_DrawFrame(void) {
	size_t doomw = DOOMGENERIC_RESX;
	size_t doomh = DOOMGENERIC_RESY;
	size_t doompitch = doomw * 4;
	for (int y = 0; y < DOOMGENERIC_RESY; y++) {
		memcpy(fb.b + fb.pitch * y, ((char*)DG_ScreenBuffer) + doompitch * y, doompitch);
	}

	struct rect d;
	dirty_mark(&d, 0, 0);
	dirty_mark(&d, doomw, doomh);
	dirty_flush(&d, &fb);
}

void DG_SleepMs(uint32_t ms) {
	_sys_sleep(ms);
}

uint32_t DG_GetTicksMs(void) {
	uint32_t ret = _sys_time(0) / 1000000;
	return ret;
}

int DG_GetKey(int *pressed, unsigned char *key) {
	uint8_t c;
	int read = ring_get(&kb_backlog, (char*)&c, 1);
	bool extended = c == 0xe0;;
	if (read == 0) return 0;
	if (extended) ring_get(&kb_backlog, (char*)&c, 1);
	bool down = !(c & 0x80);

	c &= 0x7f;
	if (!extended) {
		switch (c) {
			case 0x01:
				c = KEY_ESCAPE;
				break;
			case 0x2a: case 0x36:
				c = KEY_RSHIFT;
				break;
			case 0x1d:
				c = KEY_FIRE;
				break;
			default:
				c = keymap_lower[c & 0x7f];
				switch (c) {
					case ' ': c = KEY_USE; break;
				}
		}
	} else {
		switch (c) {
			case 0x48: c = KEY_UPARROW; break;
			case 0x50: c = KEY_DOWNARROW; break;
			case 0x4B: c = KEY_LEFTARROW; break;
			case 0x4D: c = KEY_RIGHTARROW; break;
			default:   c = '\0'; break;
		}
	}
	*pressed = down;
	*key = c;
	return 1;
}

void DG_SetWindowTitle(const char *title) {
	(void)title;
}

int main(int argc, char **argv) {
	doomgeneric_Create(argc, argv);
	for (;;) {
		doomgeneric_Tick();
	}
}

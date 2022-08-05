#include "vterm.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct point cursor = {0};

void in_char(char c) {
	switch (c) {
		case '\n':
			cursor.x = 0;
			cursor.y++;
			break;
		case '\b':
			if (cursor.x > 0) cursor.x--;
			break;
		default:
			font_blit(c, cursor.x, cursor.y);
			cursor.x++;
	}

	if (cursor.x * font.w >= fb.width) {
		cursor.x = 0;
		cursor.y++;
	}
	while (cursor.y * font.h >= fb.height) scroll();
}

int main();

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
	font_load("/init/font.psf");

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

	return 1;
}

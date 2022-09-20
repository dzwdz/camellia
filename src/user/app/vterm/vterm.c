#include "vterm.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/compat.h>

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
	while ((cursor.y + 1) * font.h >= fb.height) scroll();
}

int main(void) {
	if (fb_setup(&fb, "/kdev/video/") < 0) {
		eprintf("fb_setup error");
		return 1;
	}
	font_load("/init/font.psf");

	static char buf[512];
	struct ufs_request res;
	while (!c0_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				// TODO check path
				c0_fs_respond(NULL, 0, 0);
				break;

			case VFSOP_WRITE:
				if (res.flags) {
					c0_fs_respond(NULL, -1, 0);
				} else {
					for (size_t i = 0; i < res.len; i++)
						in_char(buf[i]);
					dirty_flush(&dirty, &fb);
					c0_fs_respond(NULL, res.len, 0);
				}
				break;

			default:
				c0_fs_respond(NULL, -1, 0);
				break;
		}
	}

	return 1;
}

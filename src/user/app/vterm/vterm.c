#include "vterm.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdlib.h>

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

int main(void) {
	fb_fd = _syscall_open("/kdev/video/b", 13, 0);
	// TODO don't hardcode size
	fb.len = 640 * 480 * 4;
	fb.width = 640;
	fb.height = 480;
	fb.pitch = 640 * 4;
	fb.b = malloc(fb.len);

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

#include "vterm.h"
#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <string.h>

struct framebuf fb;
struct rect dirty;

void scroll(void) {
	size_t row_len = fb.pitch * font.h;
	memmove(fb.b, fb.b + row_len, fb.len - row_len);
	memset(fb.b + fb.len - row_len, 0, row_len);
	cursor.y--;

	dirty.x1 =  0; dirty.y1 =  0;
	dirty.x2 = ~0; dirty.y2 = ~0;
}

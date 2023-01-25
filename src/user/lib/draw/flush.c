#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <user/lib/draw/draw.h>

static void flush_combined(struct rect pix, struct framebuf *fb) {
	size_t low  = fb->pitch * pix.y1 + 4 * pix.x1;
	size_t high = fb->pitch * pix.y2 + 4 * pix.y2 + 4;
	_sys_write(fb->fd, fb->b + low, high - low, low, 0);
}

static void flush_split(struct rect pix, struct framebuf *fb) {
	static uint64_t execbuf[EXECBUF_MAX_LEN / sizeof(uint64_t)];
	size_t epos = 0;
	if (7 * (pix.y2 - pix.y1) * sizeof(uint64_t) >= sizeof execbuf) {
		flush_combined(pix, fb);
		return;
	}

	for (uint32_t y = pix.y1; y < pix.y2; y++) {
		size_t low  = fb->pitch * y + 4 * pix.x1;
		size_t high = fb->pitch * y + 4 * pix.x2 + 4;

		execbuf[epos++] = EXECBUF_SYSCALL;
		execbuf[epos++] = _SYS_WRITE;
		execbuf[epos++] = fb->fd;
		execbuf[epos++] = (uintptr_t)fb->b + low;
		execbuf[epos++] = high - low;
		execbuf[epos++] = low;
		execbuf[epos++] = 0;
	}
	_sys_execbuf(execbuf, epos * sizeof(uint64_t));
}

void dirty_flush(struct rect *d, struct framebuf *fb) {
	if (~d->x1 == 0) return;
	if (d->x2 >= fb->width)  d->x2 = fb->width - 1;
	if (d->y2 >= fb->height) d->y2 = fb->height - 1;

	/* the threshold is mostly arbitrary, wasn't based on any real benchmarks */
	if (d->x2 - d->x1 > fb->width - 600) flush_combined(*d, fb);
	else flush_split(*d, fb);
	dirty_reset(d);
}

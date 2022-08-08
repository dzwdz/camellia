#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/draw/draw.h>

void dirty_reset(struct rect *d) {
	d->x1 = ~0; d->y1 = ~0;
	d->x2 =  0; d->y2 =  0;
}

void dirty_mark(struct rect *d, uint32_t x, uint32_t y) {
	if (d->x1 > x) d->x1 = x;
	if (d->x2 < x) d->x2 = x;
	if (d->y1 > y) d->y1 = y;
	if (d->y2 < y) d->y2 = y;
}

int fb_setup(struct framebuf *fb, const char *base) {
	char path[64], *spec;
	size_t pos;
	FILE *f;

	f = fopen(base, "r");
	if (!f) return -errno;

	pos = strlen(base);
	memcpy(path, base, pos);
	spec = path + pos;
	fread(spec, 1, sizeof(path) - pos, f);
	/* assumes the read went correctly */
	fclose(f);

	fb->fd = _syscall_open(path, strlen(path), 0);
	if (fb->fd < 0) return fb->fd;

	fb->width  = strtol(spec, &spec, 0);
	if (*spec++ != 'x') return -EINVAL;
	fb->height = strtol(spec, &spec, 0);
	if (*spec++ != 'x') return -EINVAL;
	fb->bpp = strtol(spec, &spec, 0);
	if (fb->bpp != 32) return -EINVAL;

	fb->len = _syscall_getsize(fb->fd);
	fb->pitch = fb->len / fb->height;
	fb->b = malloc(fb->len);

	_syscall_read(fb->fd, fb->b, fb->len, 0);

	return 0;
}

int fb_anon(struct framebuf *fb, size_t w, size_t h) {
	fb->width  = w;
	fb->height = h;
	fb->bpp = 32;
	fb->pitch = fb->width * fb->bpp / 8;
	fb->len = fb->pitch * fb->height;
	fb->b = malloc(fb->len);
	fb->fd = -1;
	return 0;
}

uint32_t *fb_pixel(struct framebuf *fb, uint32_t x, uint32_t y) {
	if (x < fb->width && y < fb->height)
		return (void*)fb->b + fb->pitch * y + 4 * x;
	return NULL;
}

void fb_cpy(
	struct framebuf *dest, const struct framebuf *src,
	size_t xd, size_t yd, size_t xs, size_t ys, size_t w, size_t h)
{
	for (size_t y = 0; y < h; y++) {
		memcpy(
			fb_pixel(      dest, xd, yd + y),
			fb_pixel((void*)src, xs, ys + y),
			w * 4
		);
	}
}

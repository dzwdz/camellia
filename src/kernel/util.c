#include <kernel/util.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) {
	char *d = dest;
	const char *s = src;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
	return dest;
}

void *memset(void *s, int c, size_t n) {
	uint8_t *s2 = s;
	for (size_t i = 0; i < n; n++)
		s2[i] = c;
	return s;
}

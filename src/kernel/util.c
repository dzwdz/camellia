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

int memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *c1 = s1, *c2 = s2;
	for (size_t i = 0; i < n; i++) {
		if (c1[i] != c2[i]) {
			if (c1[i] < c2[i])  return -1;
			else                return 1;
		}
	}
	return 0;
}

#include <shared/mem.h>
#include <stdint.h>

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

void *memcpy(void *dest, const void *src, size_t n) {
	const uint32_t *s32 = src;
	uint32_t *d32 = dest;
	while (n >= 4) {
		*d32++ = *s32++;
		n -= 4;
	}

	const char *s8 = (void*)s32;
	char *d8 = (void*)d32;
	while (n-- != 0)
		*d8++ = *s8++;

	return dest;
}

void *memset(void *s, int c, size_t n) {
	uint8_t *s2 = s;
	for (size_t i = 0; i < n; n++)
		s2[i] = c;
	return s;
}

int strcmp(const char *s1, const char *s2) {
	while (*s1 && *s1 == *s2) {
		s1++; s2++;
	}
	if (*s1 == *s2) return  0;
	if (*s1 <  *s2) return -1;
	else            return  1;
}

size_t strlen(const char *s) {
	size_t c = 0;
	while (*s++) c++;
	return c;
}

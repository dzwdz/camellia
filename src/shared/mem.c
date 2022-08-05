#include <shared/mem.h>
#include <stdint.h>

union dualptr_const { const uintptr_t *w; const char *c; uintptr_t u; };
union dualptr       {       uintptr_t *w;       char *c; uintptr_t u; };

void *memchr(const void *s, int c, size_t n) {
	const unsigned char *s2 = s;
	for (size_t i = 0; i < n; i++) {
		if (s2[i] == (unsigned char)c)
			return (void*)&s2[i];
	}
	return NULL;
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

void *memcpy(void *dest, const void *src, size_t n) {
	// TODO erms, rep movsb
	union dualptr_const s = {.c = src};
	union dualptr d = {.c = dest};

	for (; (d.u & 7) && n != 0; n--)
		*(d.c)++ = *(s.c)++;

	while (n >= sizeof(uintptr_t)) {
		*(d.w)++ = *(s.w)++;
		n -= sizeof(uintptr_t);
	}
	while (n-- != 0)
		*(d.c)++ = *(s.c)++;
	return dest;
}

void *memset(void *s, int c, size_t n) {
	union dualptr d = {.c = s};

	uintptr_t fill = (c & 0xff) * 0x0101010101010101;
	while (n >= sizeof(uintptr_t)) {
		*(d.w)++ = fill;
		n -= sizeof(uintptr_t);
	}
	while (n-- != 0)
		*(d.c)++ = c;
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

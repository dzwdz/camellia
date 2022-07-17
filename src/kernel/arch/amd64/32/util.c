#include <kernel/arch/amd64/32/util.h>

void *memset32(void *s, int c, size_t n) {
	unsigned char *s2 = s;
	for (size_t i = 0; i < n; i++)
		s2[i] = c;
	return s;
}

#include <kernel/util.h>
#include <stdint.h>

void *memset(void *s, int c, size_t n) {
	uint8_t *s2 = s;
	for (size_t i = 0; i < n; n++)
		s2[i] = c;
	return s;
}

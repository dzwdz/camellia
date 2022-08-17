#include "util.h"

void hexdump(const void *vbuf, size_t len) {
	const uint8_t *buf = vbuf;
	for (size_t i = 0; i < len; i += 16) {
		printf("%08x  ", i);

		for (size_t j = i; j < i + 8 && j < len; j++)
			printf("%02x ", buf[j]);
		printf(" ");
		for (size_t j = i + 8; j < i + 16 && j < len; j++)
			printf("%02x ", buf[j]);
		printf("\n");
	}
}

/* https://www.w3.org/TR/PNG/#D-CRCAppendix */
static uint32_t crc_table[256];
uint32_t crc32(const uint8_t *buf, size_t len) {
	if (!crc_table[1]) {
		for (int i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int j = 0; j < 8; j++)
				c = ((c&1) ? 0xedb88320 : 0) ^ (c >> 1);
			crc_table[i] = c;
		}
	}

	uint32_t c = 0xFFFFFFFF;
	for (size_t i = 0; i < len; i++)
		c = crc_table[(c ^ buf[i]) & 0xff] ^ (c >> 8);
	return ~c;
}

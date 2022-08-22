#include "util.h"

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

uint16_t ip_checksum(const uint8_t *buf, size_t len) {
	uint32_t c = 0;
	while (len >= 2) {
		c += nget16(buf);
		buf += 2; len -= 2;
	}
	if (len) c += (*buf) << 8;
	while (c > 0xFFFF)
		c = (c & 0xFFFF) + (c >> 16);
	return ~c;
}

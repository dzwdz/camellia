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
	while (c > 0xFFFF) c = (c & 0xFFFF) + (c >> 16);
	return ~c;
}

uint16_t ip_checksumphdr(
	const uint8_t *buf, size_t len,
	uint32_t ip1, uint32_t ip2,
	uint16_t proto)
{
	uint32_t c = (uint16_t)~ip_checksum(buf, len);
	c += (ip1 & 0xFFFF) + (ip1 >> 16);
	c += (ip2 & 0xFFFF) + (ip2 >> 16);
	c += proto + len;
	while (c > 0xFFFF) c = (c & 0xFFFF) + (c >> 16);
	return ~c;
}

int ip_parse(const char *s, uint32_t *dest) {
	if (!s) return -1;

	uint32_t ip = strtol(s, (char**)&s, 0);
	int parts = 1;
	for (; parts < 4; parts++) {
		if (!*s) break;
		if (*s++ != '.') return -1;
		ip <<= 8;
		ip += strtol(s, (char**)&s, 0);
	}
	if (parts > 1)
		ip = ((ip & 0xFFFFFF00) << 8 * (4 - parts)) | (ip & 0xFF);
	*dest = ip;
	return 0;
}

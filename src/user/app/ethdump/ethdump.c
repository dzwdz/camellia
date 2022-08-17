#include <camellia/syscalls.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, "ethdump: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

static void hexdump(void *vbuf, size_t len) {
	uint8_t *buf = vbuf;
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

static void parse_icmp(const uint8_t *buf, size_t len) {
	if (len < 4) return;
	uint8_t type = buf[0];
	switch (type) {
		case 8:
			printf("icmp type %u - echo request\n", type);
			break;
		default:
			printf("icmp type %u - unknown\n", type);
			break;
	}
}

static void parse_ipv4(const uint8_t *buf, size_t len) {
	uint8_t version, headerlen, proto;
	uint16_t packetlen, id, fragoffset;
	uint32_t dest, src;

	version   = buf[0] >> 4;
	if (version != 4) {
		printf("bad IPv4 version %u\n", version);
		return;
	}
	headerlen = (buf[0] & 0xf) * 4;
	packetlen = (buf[2] << 8) | buf[3];
	id        = (buf[4] << 8) | buf[5];
	proto     = buf[9];
	src  = (buf[12] << 24)
	     | (buf[13] << 16)
	     | (buf[14] <<  8)
	     | (buf[15] <<  0);
	dest = (buf[16] << 24)
	     | (buf[17] << 16)
	     | (buf[18] <<  8)
	     | (buf[19] <<  0);

	// TODO checksum
	// TODO fragmentation

	printf("headerlen %u, packetlen %u (real %u), id %u\n", headerlen, packetlen, len, id);
	printf("from %x to %x\n", src, dest);
	printf("id %u\n", id);
	if (packetlen < headerlen) {
		printf("headerlen too big\n");
		return;
	}
	switch (proto) {
		case 1:
			printf("proto %u - icmp\n", proto);
			parse_icmp(buf + headerlen, packetlen - headerlen);
			break;
		default:
			printf("proto %u - unknown\n", proto);
			break;
	}
}

/* https://www.w3.org/TR/PNG/#D-CRCAppendix */
uint32_t crc_table[256];
static uint32_t crc32(const uint8_t *buf, size_t len) {
	if (!crc_table[1]) {
		eprintf("building crc cache");
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

static void parse_ethernet(const uint8_t *buf, size_t len) {
	uint8_t dmac[6], smac[6];

	if (len < 60) return;
	for (int i = 0; i < 6; i++) dmac[i] = buf[i];
	for (int i = 0; i < 6; i++) smac[i] = buf[i + 6];
	printf("from %02x:%02x:%02x:%02x:%02x:%02x\n",
		smac[0], smac[1], smac[2], smac[3], smac[4], smac[5]);
	printf("to   %02x:%02x:%02x:%02x:%02x:%02x\n",
		dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

	/* byte order switched on purpose */
	uint32_t crc = (buf[len - 1] << 24)
	             | (buf[len - 2] << 16)
	             | (buf[len - 3] <<  8)
	             | (buf[len - 4] <<  0);
	printf("fcf %x, crc %x\n", crc, crc32(buf, len - 4));

	uint16_t ethertype = (buf[12] << 8) | buf[13];
	if (ethertype == 0x800) {
		printf("ethertype %u - IPv4\n", ethertype);
		parse_ipv4(buf + 14, len - 14);
	} else {
		printf("ethertype %u - unknown\n", ethertype);
	}
}

static void dummy_packet(handle_t h) {
	const uint8_t packet[] = {
		/* raw dump of a ICMP broadcast ping */
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x52, 0x54,
		0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
		0x00, 0x54, 0x06, 0xa6, 0x40, 0x00, 0x40, 0x01,
		0x73, 0x5a, 0xc0, 0xa8, 0x00, 0x01, 0xff, 0xff,
		0xff, 0xff, 0x08, 0x00, 0x82, 0x62, 0xef, 0x07,
		0x00, 0xcb, 0x3b, 0x83, 0x4a, 0x47, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		/* 0x20, 0xcf, 0x98, 0x7f, */ /* crc */
	};
	_syscall_write(h, packet, sizeof packet, 0, 0);
}

int main(void) {
	const char *path = "/kdev/eth";
	handle_t h = _syscall_open(path, strlen(path), 0);
	if (h < 0) {
		eprintf("couldn't open %s", path);
		return 1;
	}

	for (int i = 0; i < 20; i++)
		dummy_packet(h);
	return 0;

	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _syscall_read(h, buf, buflen, -1);
		if (ret < 0) break;
		printf("\npacket of length %u\n", ret);
		hexdump(buf, ret);
		parse_ethernet((void*)buf, ret);
	}
	return 0;
}

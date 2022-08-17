#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, "ethdump: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

handle_t eth_h;

// TODO dynamically get mac
uint8_t my_mac[] = {0x52, 0x54, 0x00, 0xCA, 0x77, 0x1A};
uint32_t my_ip = (192 << 24) + (168 << 16) + 11;

static void hexdump(const void *vbuf, size_t len) {
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

static void nput16(void *vbuf, uint16_t n) {
	uint8_t *b = vbuf;
	b[0] = n >> 8;
	b[1] = n >> 0;
}

static void nput32(void *vbuf, uint32_t n) {
	uint8_t *b = vbuf;
	b[0] = n >> 24;
	b[1] = n >> 16;
	b[2] = n >>  8;
	b[3] = n >>  0;
}

static uint16_t nget16(const void *vbuf) {
	const uint8_t *b = vbuf;
	return (b[0] << 8)
	     | (b[1] << 0);
}

static uint32_t nget32(const void *vbuf) {
	const uint8_t *b = vbuf;
	return (b[0] << 24)
	     | (b[1] << 16)
	     | (b[2] <<  8)
	     | (b[3] <<  0);
}


static void parse_icmp(const uint8_t *buf, size_t len) {
	if (len < 4) return;
	uint8_t type = buf[0];
	printf("ICMP type %u\n", type);
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
	packetlen = nget16(buf + 2);
	id        = nget16(buf + 4);
	proto     = buf[9];
	src  = nget32(buf + 12);
	dest = nget32(buf + 16);

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

static void parse_arp(const uint8_t *buf, size_t len) {
	// TODO no bound checks
	uint16_t htype = nget16(buf + 0);
	uint16_t ptype = nget16(buf + 2);
	uint16_t op = nget16(buf + 6);

	const char *ops = "bad operation";
	if (op == 1) ops = "request";
	if (op == 2) ops = "reply";

	printf("ARP htype 0x%x, ptype 0x%x, %s\n", htype, ptype, ops);

	if (!(htype == 1 && ptype == 0x800)) return;
	/* IPv4 over Ethernet */

	if (op != 1) return;
	/* a request */

	uint32_t daddr = nget32(buf + 24);
	printf("IPv4 request for %08x\n", daddr);

	if (daddr == my_ip) {/* send a response */
		char res[64];
		memset(res, 0, 64);
		/* Ethernet */
		memcpy(res + 0, buf + 8, 6); /* from ARP sender MAC */
		memcpy(res + 6, my_mac,  6);
		nput16(res + 12, 0x0806); /* ethertype == ARP */

		/* ARP */
		nput16(res + 14, 1);      /* Ethernet */
		nput16(res + 16, 0x800);  /* IPv4 */
		res[18] = 6; res[19] = 4; /* address lengths */
		nput16(res + 20, 2);      /* operation == reply */
		memcpy(res + 22, my_mac, 6);
		nput32(res + 28, my_ip);
		memcpy(res + 32, buf + 8, 10); /* sender's MAC and IP */
		printf("sending ARP response:\n");
		hexdump(res, sizeof res);
		_syscall_write(eth_h, res, sizeof res, 0, 0);
	}
}

/* https://www.w3.org/TR/PNG/#D-CRCAppendix */
uint32_t crc_table[256];
static uint32_t crc32(const uint8_t *buf, size_t len) {
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

static void parse_ethernet(const uint8_t *buf, size_t len) {
	uint8_t dmac[6], smac[6];

	if (len < 60) return;
	for (int i = 0; i < 6; i++) dmac[i] = buf[i];
	for (int i = 0; i < 6; i++) smac[i] = buf[i + 6];
	printf("from %02x:%02x:%02x:%02x:%02x:%02x\n",
		smac[0], smac[1], smac[2], smac[3], smac[4], smac[5]);
	printf("to   %02x:%02x:%02x:%02x:%02x:%02x\n",
		dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

	if (false) {
		/* byte order switched on purpose */
		uint32_t crc = (buf[len - 1] << 24)
		             | (buf[len - 2] << 16)
		             | (buf[len - 3] <<  8)
		             | (buf[len - 4] <<  0);
		printf("fcf %x, crc %x\n", crc, crc32(buf, len - 4));
	}

	uint16_t ethertype = nget16(buf + 12);
	printf("ethertype %u\n", ethertype);
	switch (ethertype) {
		case 0x800:
			parse_ipv4(buf + 14, len - 14);
			break;
		case 0x806:
			parse_arp(buf + 14, len - 14);
			break;
		default:
			printf("(unknown)\n");
			break;
	}
}

int main(void) {
	const char *path = "/kdev/eth";
	eth_h = _syscall_open(path, strlen(path), 0);
	if (eth_h < 0) {
		eprintf("couldn't open %s", path);
		return 1;
	}

	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _syscall_read(eth_h, buf, buflen, -1);
		if (ret < 0) break;
		printf("\npacket of length %u\n", ret);
		hexdump(buf, ret);
		parse_ethernet((void*)buf, ret);
	}
	free(buf);
	return 0;
}

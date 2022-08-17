#include "proto.h"
#include "util.h"

enum {
	Version = 0,
	HdrLen  = 0,
	PktLen  = 2,
	Id      = 4,
	Proto   = 9,
	SrcIP   = 12,
	DestIP  = 16,
};

void ipv4_parse(const uint8_t *buf, size_t len) {
	uint8_t version, headerlen, proto;
	uint16_t packetlen, id;
	uint32_t dest, src;

	version   = buf[Version] >> 4;
	if (version != 4) {
		printf("bad IPv4 version %u\n", version);
		return;
	}
	headerlen = (buf[HdrLen] & 0xf) * 4;
	packetlen = nget16(buf + PktLen);
	id        = nget16(buf + Id);
	proto     = buf[Proto];
	src  = nget32(buf + SrcIP);
	dest = nget32(buf + DestIP);

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
			icmp_parse(buf + headerlen, packetlen - headerlen);
			break;
		default:
			printf("proto %u - unknown\n", proto);
			break;
	}
}

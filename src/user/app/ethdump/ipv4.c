#include "proto.h"
#include "util.h"
#include <stdlib.h>

enum {
	Version  = 0,
	HdrLen   = 0,
	PktLen   = 2,
	Id       = 4,
	TTL      = 8,
	Proto    = 9,
	Checksum = 10,
	SrcIP    = 12,
	DstIP    = 16,
};

void ipv4_parse(const uint8_t *buf, size_t len, struct ethernet ether) {
	uint8_t version, headerlen, proto;
	uint16_t packetlen, id;
	uint32_t dst, src;

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
	dst = nget32(buf + DstIP);

	// TODO checksum
	// TODO fragmentation

	printf("headerlen %u, packetlen %u (real %u), id %u\n", headerlen, packetlen, len, id);
	printf("from %x to %x\n", src, dst);
	printf("id %u\n", id);
	if (packetlen < headerlen) {
		printf("headerlen too big\n");
		return;
	}

	struct ipv4 ip = (struct ipv4){
		.e = ether,
		.src = src,
		.dst = dst,
		.proto = proto,
	};

	switch (proto) {
		case 1:
			printf("proto %u - icmp\n", proto);
			icmp_parse(buf + headerlen, packetlen - headerlen, ip);
			break;
		default:
			printf("proto %u - unknown\n", proto);
			break;
	}
}

uint8_t *ipv4_start(size_t len, struct ipv4 ip) {
	ip.e.type = ET_IPv4;
	if (!ip.src) ip.src = state.ip;
	if (!ip.e.dst && ip.dst == 0xFFFFFFFF)
		ip.e.dst = &MAC_BROADCAST;

	size_t hdrlen = 20;
	uint8_t *pkt = ether_start(len + hdrlen, ip.e);
	pkt[Version] = 0x40;
	pkt[HdrLen] |= hdrlen / 4;
	nput16(pkt + PktLen, len + hdrlen);
	pkt[TTL] = 0xFF;
	pkt[Proto] = ip.proto;
	nput32(pkt + SrcIP, ip.src);
	nput32(pkt + DstIP, ip.dst);

	nput16(pkt + Checksum, ip_checksum(pkt, hdrlen));

	return pkt + hdrlen;
}
void ipv4_finish(uint8_t *pkt) {
	ether_finish(pkt - 20);
}

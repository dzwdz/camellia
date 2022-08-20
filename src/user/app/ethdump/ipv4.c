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
	uint8_t version, headerlen;
	uint16_t packetlen, id;

	version   = buf[Version] >> 4;
	if (version != 4) return;
	headerlen = (buf[HdrLen] & 0xf) * 4;
	packetlen = nget16(buf + PktLen);
	if (packetlen < headerlen) return;
	id        = nget16(buf + Id);

	// TODO checksum
	// TODO fragmentation

	struct ipv4 ip = (struct ipv4){
		.e = ether,
		.src = nget32(buf + SrcIP),
		.dst = nget32(buf + DstIP),
		.proto = buf[Proto],
	};

	switch (ip.proto) {
		case 1:
			icmp_parse(buf + headerlen, packetlen - headerlen, ip);
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

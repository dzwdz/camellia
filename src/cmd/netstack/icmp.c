#include "proto.h"
#include "util.h"

enum {
	Type     = 0,
	Code     = 1,
	Checksum = 2,
	Payload  = 4,
};

void icmp_parse(const uint8_t *buf, size_t len, struct ipv4 ip) {
	if (len < Payload) return;
	uint8_t type = buf[Type];
	if (type == 8 && ip.dst == state.ip) {
		/* echo reply */
		icmp_send(buf + Payload, len - Payload, (struct icmp){
			.type = 0,
			.ip.dst = ip.src,
			.ip.e.dst = ip.e.src,
		});
	}
}

void icmp_send(const void *payload, size_t len, struct icmp i) {
	i.ip.proto = 1;
	uint8_t *pkt = malloc(Payload + len);
	pkt[Type] = i.type;
	pkt[Code] = i.code;
	memcpy(pkt + Payload, payload, len);
	nput16(pkt + Checksum, 0);
	nput16(pkt + Checksum, ip_checksum(pkt, Payload + len));
	ipv4_send(pkt, Payload + len, i.ip);
	free(pkt);
}

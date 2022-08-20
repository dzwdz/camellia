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
		uint8_t *pkt = icmp_start(len - Payload, (struct icmp){
			.type = 0,
			.ip.dst = ip.src,
			.ip.e.dst = ip.e.src,
		});
		memcpy(pkt, buf + Payload, len - Payload);
		icmp_finish(pkt, len - Payload);
	}
}

uint8_t *icmp_start(size_t len, struct icmp i) {
	i.ip.proto = 1;
	uint8_t *pkt = ipv4_start(Payload + len, i.ip);
	pkt[Type] = i.type;
	pkt[Code] = i.code;
	return pkt + Payload;
}
void icmp_finish(uint8_t *pkt, size_t len) {
	pkt -= Payload;
	nput16(pkt + Checksum, ip_checksum(pkt, Payload + len));
	ipv4_finish(pkt);
}

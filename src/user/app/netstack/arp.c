#include "proto.h"
#include "util.h"
#include <string.h>

enum {
	HdrType   = 0,
		HdrTypeEther = 1,
	ProtoType = 2,
	HdrALen   = 4,
	ProtoALen = 5,
	Operation = 6,
		OpReq   = 1,
		OpReply = 2,
};

void arp_parse(const uint8_t *buf, size_t len) {
	// TODO no bound checks
	uint16_t htype = nget16(buf + HdrType);
	uint16_t ptype = nget16(buf + ProtoType);
	uint16_t op = nget16(buf + Operation);

	if (!(htype == HdrTypeEther && ptype == ET_IPv4)) return;
	enum { /* only valid for this combination of header + proto */
		SrcMAC =  8,
		SrcIP  = 14,
		DstMAC = 18,
		DstIP  = 24,
	};

	if (op == OpReq) {
		uint32_t daddr = nget32(buf + DstIP);
		if (daddr == state.ip) {
			uint8_t *pkt = ether_start(30, (struct ethernet){
				.dst = buf + SrcMAC,
				.type = ET_ARP,
			});
			nput16(pkt + HdrType, 1);
			nput16(pkt + ProtoType, ET_IPv4);
			pkt[HdrALen] = 6;
			pkt[ProtoALen] = 4;
			nput16(pkt + Operation, OpReply);
			memcpy(pkt + SrcMAC, state.mac, 6);
			nput32(pkt + SrcIP, state.ip);
			memcpy(pkt + DstMAC, buf + SrcMAC, 10); /* sender's MAC and IP */
			ether_finish(pkt);
		}
	}
}

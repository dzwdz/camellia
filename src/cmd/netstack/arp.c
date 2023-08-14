#include "proto.h"
#include "util.h"
#include <assert.h>
#include <camellia/syscalls.h>
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

struct arpc {
	struct arpc *next;
	uint32_t ip;
	mac_t mac;
};
static struct arpc *arpcache;
static void arpcache_put(uint32_t ip, mac_t mac);

void arp_parse(const uint8_t *buf, size_t len) {
	if (len < Operation + 2) return;
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
	if (len < DstIP + 4) return;
	arpcache_put(nget32(buf + SrcIP), *(mac_t*)buf + SrcMAC);

	if (op == OpReq) {
		uint32_t daddr = nget32(buf + DstIP);
		if (daddr == state.ip) {
			uint8_t *pkt = ether_start(30, (struct ethernet){
				.dst = (void*)(buf + SrcMAC),
				.type = ET_ARP,
			});
			nput16(pkt + HdrType, HdrTypeEther);
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

void arp_request(uint32_t ip) {
	enum {
		SrcMAC =  8,
		SrcIP  = 14,
		DstMAC = 18,
		DstIP  = 24,
	};
	uint8_t *pkt = ether_start(28, (struct ethernet){
		.src = &state.mac,
		.dst = &MAC_BROADCAST,
		.type = ET_ARP,
	});
	nput16(pkt + HdrType, HdrTypeEther);
	nput16(pkt + ProtoType, ET_IPv4);
	pkt[HdrALen] = 6;
	pkt[ProtoALen] = 4;
	nput16(pkt + Operation, OpReq);
	memcpy(pkt + SrcMAC, state.mac, 6);
	nput32(pkt + SrcIP, state.ip);
	memcpy(pkt + DstMAC, &MAC_BROADCAST, 6);
	nput32(pkt + DstIP, ip);
	ether_finish(pkt);
}

static void arpcache_put(uint32_t ip, mac_t mac) {
	for (struct arpc *iter = arpcache; iter; iter = iter->next) {
		if (memcmp(iter->mac, mac, 6) == 0) {
			if (iter->ip == ip) return; /* cache entry correct */
			else break; /* cache entry needs updating */
		}
	}
	struct arpc *e = malloc(sizeof *e);
	e->next = arpcache;
	e->ip = ip;
	memcpy(e->mac, mac, 6);
	arpcache = e;
}

int arpcache_get(uint32_t ip, mac_t *mac) {
	for (struct arpc *iter = arpcache; iter; iter = iter->next) {
		if (iter->ip == ip) {
			if (mac) memcpy(mac, iter->mac, 6);
			return 0;
		}
	}
	return -1;
}

void arp_fsread(hid_t h, long offset) {
	const char *fmt = "%08x\t%02x:%02x:%02x:%02x:%02x:%02x\n";
	long linelen = snprintf(NULL, 0, fmt, 0, 1, 2, 3, 4, 5, 6) + 1;
	char buf[28];
	assert(linelen <= (long)sizeof(buf));
	if (offset < 0) goto err;

	struct arpc *cur = arpcache;
	if (!cur) goto err;
	for (; linelen <= offset; offset -= linelen) {
		cur = cur->next;
		if (!cur) goto err;
	}
	assert(0 <= offset && offset < linelen);

	snprintf(buf, sizeof buf, fmt, cur->ip,
		cur->mac[0],
		cur->mac[1],
		cur->mac[2],
		cur->mac[3],
		cur->mac[4],
		cur->mac[5]);
	_sys_fs_respond(h, buf + offset, linelen - offset, 0);
	return;
err:
	_sys_fs_respond(h, NULL, -1, 0);
}

long arp_fswrite(const char *buf, long len, long offset) {
	if (offset != -1) return -1;
	uint32_t ip;
	char tmp[16];
	size_t iplen = len < 15 ? len : 15;
	memcpy(tmp, buf, iplen);
	tmp[iplen] = '\0';
	if (ip_parse(tmp, &ip) < 0) {
		return -1;
	} else {
		arp_request(ip);
		return len;
	}
}

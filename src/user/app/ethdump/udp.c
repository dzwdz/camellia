#include "proto.h"
#include "util.h"

enum {
	SrcPort  = 0,
	DstPort  = 2,
	Length   = 4,
	Checksum = 6,
	Payload  = 8,
};


struct udp_conn {
	uint32_t lip, rip;
	uint16_t lport, rport;
	mac_t rmac;
	void (*on_conn)(struct udp_conn *, void *); /* if non-NULL - listening, if NULL - estabilished */
	void (*on_recv)(const void *, size_t, void *);
	void *carg;
	struct udp_conn *next, **prev;
};
struct udp_conn *conns;
void udp_listen(uint16_t port,
	void (*on_conn)(struct udp_conn *, void *carg),
	void (*on_recv)(const void *, size_t, void *carg),
	void *carg)
{
	if (!on_conn) return;
	struct udp_conn *c = malloc(sizeof *c);
	c->lport = port;
	c->lip = state.ip;
	c->on_conn = on_conn;
	c->on_recv = on_recv;
	c->carg = carg;

	c->next = conns;
	if (c->next) *(c->next->prev) = c;
	c->prev = &conns;
	*c->prev = c;
}
void udpc_close(struct udp_conn *c) {
	if (c->next) c->next->prev = c->prev;
	*(c->prev) = c->next;
	free(c);
}
void udpc_send(struct udp_conn *c, const void *buf, size_t len) {
	uint8_t *pkt = malloc(Payload + len);
	nput16(pkt + SrcPort, c->lport);
	nput16(pkt + DstPort, c->rport);
	nput16(pkt + Length, Payload + len);
	nput16(pkt + Checksum, 0);
	memcpy(pkt + Payload, buf, len);
	ipv4_send(pkt, Payload + len, (struct ipv4){
		.proto = 0x11,
		.src = c->lip,
		.dst = c->rip,
		.e.dst = c->rmac,
	});
	free(pkt);
}


void udp_parse(const uint8_t *buf, size_t len, struct ipv4 ip) {
	uint16_t srcport = nget16(buf + SrcPort);
	uint16_t dstport = nget16(buf + DstPort);
	bool active = false;

	for (struct udp_conn *iter = conns; iter; iter = iter->next) {
		if (iter->on_conn && dstport == iter->lport) {
			iter->on_conn(iter, iter->carg);
			iter->on_conn = NULL;
			iter->rport = srcport;
			memcpy(&iter->rmac, ip.e.src, sizeof(mac_t));
			iter->rip = ip.src;
		}
		if (iter->rip == ip.src
			&& iter->rport == srcport
			&& iter->lport == dstport
			&& iter->on_recv)
		{
			active = true;
			iter->on_recv(buf + Payload, len - Payload, iter->carg);
		}
	}

	if (!active) {
		uint8_t *pkt = malloc(4 + ip.hlen + 8);
		nput32(pkt, 0);
		memcpy(pkt + 4, ip.header, ip.hlen + 8);
		icmp_send(pkt, 4 + ip.hlen + 8, (struct icmp){
			.type = 3, /* destination unreachable */
			.code = 3, /* port unreachable */
			.ip.dst = ip.src,
			.ip.e.dst = ip.e.src,
		});
		free(pkt);
	}
}

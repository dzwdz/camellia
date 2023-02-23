/* Welcome to spaghetti land.
 * This is anything but production quality. It's throwaway code, meant
 * only to see how networking could fit into the architecture of the
 * system. */
#include "proto.h"
#include "util.h"
#include <assert.h>
#include <shared/container/ring.h>

enum {
	SrcPort = 0,
	DstPort = 2,
	Seq     = 4,
	AckNum  = 8, /* last processed seq + 1 */
	Flags   = 12,
		FlagSize = 0xF000, /* size of header / 4 bytes */
		/* 3 bits: 0, still unused
		 * 3 bits: modern stuff not in the original RFC */
		FlagURG = 0x0020, /* urgent */
		FlagACK = 0x0010,
		FlagPSH = 0x0008, /* force buffer flush */
		FlagRST = 0x0004, /* reset connection */
		FlagSYN = 0x0002, /* first packet; sync sequence numbers */
		FlagFIN = 0x0001, /* last packet */
		FlagAll = 0x003F,
	WinSize  = 14, /* amount of data we're willing to receive */
	Checksum = 16,
	Urgent   = 18,
	MinHdr   = 20,
};

enum tcp_state {
	LISTEN,
	SYN_SENT,
	SYN_RCVD,
	ESTABILISHED,
	LAST_ACK,
	CLOSED,
};

struct tcp_conn {
	uint32_t lip, rip;
	mac_t rmac;
	uint16_t lport, rport;
	struct tcp_conn *next, **link;

	enum tcp_state state;
	uint32_t lack, rack;
	uint32_t lseq;
	bool uclosed; /* did the user close? */

	void (*on_conn)(struct tcp_conn *, void *carg);
	void (*on_recv)(void *carg);
	void (*on_close)(void *carg);
	void *carg;

	ring_t rx;
};
static struct tcp_conn *conns;
static void conns_append(struct tcp_conn *c) {
	c->next = conns;
	if (c->next)
		c->next->link = &c->next;
	c->link = &conns;
	*c->link = c;
}
static void tcpc_sendraw(struct tcp_conn *c, uint16_t flags, const void *buf, size_t len) {
	uint8_t *pkt = malloc(MinHdr + len);
	memset(pkt, 0, MinHdr);

	nput16(pkt + SrcPort, c->lport);
	nput16(pkt + DstPort, c->rport);
	nput32(pkt + Seq, c->lseq);
	c->lseq += len;
	nput32(pkt + AckNum, c->lack);
	flags |= (MinHdr / 4) << 12;
	nput16(pkt + Flags, flags);
	nput16(pkt + WinSize, ring_avail(&c->rx));
	memcpy(pkt + MinHdr, buf, len);
	nput16(pkt + Checksum, ip_checksumphdr(pkt, MinHdr + len, c->lip, c->rip, 6));

	ipv4_send(pkt, MinHdr + len, (struct ipv4){
		.proto = 6,
		.src = c->lip,
		.dst = c->rip,
		.e.dst = &c->rmac,
	});
	free(pkt);
}
void tcp_listen(
	uint16_t port,
	void (*on_conn)(struct tcp_conn *, void *carg),
	void (*on_recv)(void *carg),
	void (*on_close)(void *carg),
	void *carg)
{
	struct tcp_conn *c = malloc(sizeof *c);
	memset(c, 0, sizeof *c);
	c->lport = port;
	c->lip = state.ip;
	c->state = LISTEN;
	c->on_conn = on_conn;
	c->on_recv = on_recv;
	c->on_close = on_close;
	c->carg = carg;
	// TODO setting the ring size super low loses every nth byte. probably a bug with ring_t itself!
	c->rx = (ring_t){malloc(4096), 4096, 0, 0};
	conns_append(c);
}
struct tcp_conn *tcpc_new(
	struct tcp t,
	void (*on_recv)(void *carg),
	void (*on_close)(void *carg),
	void *carg)
{
	struct tcp_conn *c = malloc(sizeof *c);
	memset(c, 0, sizeof *c);
	c->lip = t.ip.src ? t.ip.src : state.ip;
	c->rip = t.ip.dst;
	c->lport = t.src ? t.src : 50002; // TODO randomize source ports
	c->rport = t.dst;
	if (arpcache_get(c->rip, &c->rmac) < 0) {
		// TODO wait for ARP reply
		arp_request(c->rip);
		if (arpcache_get(state.gateway, &c->rmac) < 0) {
			eprintf("neither target nor gateway not in ARP cache, dropping");
			free(c);
			return NULL;
		}
	}

	c->state = SYN_SENT;
	c->on_recv = on_recv;
	c->on_close = on_close;
	c->carg = carg;
	c->rx = (ring_t){malloc(4096), 4096, 0, 0};
	conns_append(c);

	tcpc_sendraw(c, FlagSYN, NULL, 0);
	c->lseq++;
	return c;
}
size_t tcpc_tryread(struct tcp_conn *c, void *buf, size_t len) {
	if (!buf) return ring_used(&c->rx);
	return ring_get(&c->rx, buf, len);
}
void tcpc_send(struct tcp_conn *c, const void *buf, size_t len) {
	tcpc_sendraw(c, FlagACK | FlagPSH, buf, len);
}
static void tcpc_tryfree(struct tcp_conn *c) {
	if (c->state == CLOSED && c->uclosed) {
		if (c->next) c->next->link = c->link;
		*c->link = c->next;
		free(c->rx.buf);
		free(c);
	}
}
void tcpc_close(struct tcp_conn *c) {
	/* ONLY FOR USE BY THE USER, drops their reference */
	assert(!c->uclosed);
	c->uclosed = true;
	if (c->state != CLOSED && c->state != LAST_ACK && c->state != LISTEN) {
		tcpc_sendraw(c, FlagFIN | FlagACK, NULL, 0);
		c->state = LAST_ACK;
		c->on_conn = NULL;
		c->on_close = NULL;
	}
	tcpc_tryfree(c);
}

void tcp_parse(const uint8_t *buf, size_t len, struct ipv4 ip) {
	if (len < 20) return;
	uint16_t srcport = nget16(buf + SrcPort);
	uint16_t dstport = nget16(buf + DstPort);
	uint32_t seq     = nget32(buf + Seq);
	uint32_t acknum  = nget32(buf + AckNum);
	uint16_t flags   = nget16(buf + Flags);
	// uint16_t winsize = nget16(buf + WinSize);
	// uint16_t chksum  = nget16(buf + Checksum);
	uint16_t hdrlen = ((flags & FlagSize) >> 12) * 4;
	if (hdrlen > len) return;
	uint16_t payloadlen = len - hdrlen;

	for (struct tcp_conn *iter = conns; iter; iter = iter->next) {
		if (iter->state == CLOSED) continue;
		if (iter->lport != dstport) continue;

		if (iter->state == LISTEN && (flags & FlagAll) == FlagSYN) {
			iter->state = SYN_RCVD;
			iter->rip = ip.src;
			iter->rport = srcport;
			iter->lack = seq + 1;
			memcpy(&iter->rmac, ip.e.src, sizeof(mac_t));
			tcpc_sendraw(iter, FlagSYN | FlagACK, NULL, 0);
			iter->lseq++;
			if (iter->on_conn) iter->on_conn(iter, iter->carg);
			return;
		}

		if (iter->rip == ip.src && iter->rport == srcport) {
			// TODO doesn't handle seq/ack overflows
			if (iter->state == SYN_SENT) {
				if (flags & FlagSYN) {
					iter->state = ESTABILISHED;
					iter->lack = seq + 1;
					tcpc_sendraw(iter, FlagACK, NULL, 0);
					return;
				} else {
					// TODO resend syn?
					return;
				}
			}
			if (flags & FlagACK) {
				if (iter->rack < acknum)
					iter->rack = acknum;
				if (iter->state == LAST_ACK) {
					// TODO check if ack has correct number
					iter->state = CLOSED;
					tcpc_tryfree(iter);
					// TODO free (also after a timeout)
					return;
				}
			}
			if (iter->lack != seq && iter->lack - 1 != seq) {
				eprintf("remote seq jumped by %d", seq - iter->lack);
				tcpc_sendraw(iter, FlagACK, NULL, 0);
				return;
			}
			// TODO check if overflows window size
			if (payloadlen) {
				iter->lack = seq + payloadlen;
				ring_put(&iter->rx, buf + hdrlen, payloadlen);
				if (iter->on_recv) iter->on_recv(iter->carg);
				tcpc_sendraw(iter, FlagACK, NULL, 0);
			}
			if (flags & FlagFIN) {
				// TODO should resend the packet until an ACK is received
				// TODO duplicated in tcpc_close
				tcpc_sendraw(iter, FlagFIN | FlagACK, NULL, 0);
				iter->state = LAST_ACK;
				if (iter->on_close) iter->on_close(iter->carg);
				return;
			}
			return;
		}
	}

	if ((flags & FlagRST) == 0) {
		uint8_t *pkt = malloc(MinHdr);
		memset(pkt, 0, MinHdr);
		nput16(pkt + SrcPort, dstport);
		nput16(pkt + DstPort, srcport);
		nput32(pkt + Seq, acknum);
		nput32(pkt + AckNum, seq + 1);
		uint16_t pktflags = FlagRST | FlagACK;
		pktflags |= (MinHdr / 4) << 12;
		nput16(pkt + Flags, pktflags);
		nput16(pkt + Checksum, ip_checksumphdr(pkt, MinHdr, ip.src, ip.dst, 6));

		ipv4_send(pkt, MinHdr, (struct ipv4){
			.proto = 6,
			.src = ip.dst,
			.dst = ip.src,
			.e.dst = ip.e.src,
		});
		free(pkt);
	}
}

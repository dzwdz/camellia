#include "proto.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>

enum {
	Version  = 0,
	HdrLen   = 0,
	TotalLen = 2,
	Id       = 4,
	FragInfo = 6,
		EvilBit   = 0x8000,
		DontFrag  = 0x4000,
		MoreFrags = 0x2000,
		FragOff   = 0x1FFF,
	TTL      = 8,
	Proto    = 9,
	Checksum = 10,
	SrcIP    = 12,
	DstIP    = 16,
};
static void ipv4_dispatch(const uint8_t *buf, size_t len, struct ipv4 ip);

struct fragment {
	struct fragment *next; /* sorted */
	size_t len, offset;
	bool last;
	uint8_t buf[];
};
struct fragmented {
	/* src, dst, proto, id come from the first arrived packet
	 * and are used to tell fragmenteds apart.
	 * the rest comes from the sequentially first packet.
	 * ip.h.header points to a malloc'd buffer*/
	struct ipv4 h;

	struct fragment *first;
	struct fragmented *next, **prev; /* *(inc->prev) == inc */
	// TODO timer
};
struct fragmented *fragmenteds;
static struct fragmented *fragmented_find(struct ipv4 fraghdr);
static void fragmented_tryinsert(const uint8_t *payload, size_t plen, struct ipv4 ip);
static void fragmented_tryfinish(struct fragmented *inc);
static void fragmented_free(struct fragmented *inc);

static struct fragmented *fragmented_find(struct ipv4 fraghdr) {
	struct fragmented *inc;
	for (inc = fragmenteds; inc; inc = inc->next) {
		if (inc->h.src == fraghdr.src &&
			inc->h.dst == fraghdr.dst &&
			inc->h.proto == fraghdr.proto &&
			inc->h.id == fraghdr.id)
		{
			return inc;
		}
	}
	inc = malloc(sizeof *inc);
	memset(inc, 0, sizeof *inc);
	inc->h.src = fraghdr.src;
	inc->h.dst = fraghdr.dst;
	inc->h.proto = fraghdr.proto;
	inc->h.id = fraghdr.id;

	inc->next = fragmenteds;
	if (inc->next) inc->next->prev = &inc->next;
	inc->prev = &fragmenteds;
	*inc->prev = inc;
	return inc;
}

static void fragmented_tryinsert(const uint8_t *payload, size_t plen, struct ipv4 ip) {
	struct fragmented *inc = fragmented_find(ip);
	size_t off = (ip.fraginfo & FragOff) * 8;
	bool last = !(ip.fraginfo & MoreFrags);
	// eprintf("fragmented packet, %u + %u, part of 0x%x", off, plen, inc);

	/* find the first fragment at a bigger offset, and insert before it */
	struct fragment **insert = &inc->first;
	for (; *insert; insert = &(*insert)->next) {
		if ((*insert)->offset > off) break;
		if ((*insert)->offset == off) return; /* duplicate packet */
	}
	/* later on: frag->next = *insert;
	 * if we're the last fragment, frag->next must == NULL */
	if (last && *insert != NULL) return;

	struct fragment *frag = malloc(sizeof(struct fragment) + plen);
	frag->next = *insert;
	*insert = frag;
	frag->len = plen;
	frag->offset = off;
	frag->last = last;
	memcpy(frag->buf, payload, plen);

	if (off == 0) { /* save header */
		assert(!inc->h.header);
		void *headercpy = malloc(ip.hlen);
		memcpy(headercpy, ip.header, ip.hlen);
		inc->h = ip;
		inc->h.header = headercpy;
	}

	fragmented_tryfinish(inc);
}

static void fragmented_tryfinish(struct fragmented *inc) {
	if (inc->first->offset != 0) return;
	for (struct fragment *iter = inc->first; iter; iter = iter->next) {
		size_t iterend = iter->offset + iter->len;
		struct fragment *next = iter->next;
		if (next) {
			if (iterend < next->offset) return; /* incomplete */
			if (iterend > next->offset) {
				fragmented_free(inc);
				return;
			}
		} else if (iter->last) {
			void *buf = malloc(iterend);
			for (struct fragment *iter = inc->first; iter; iter = iter->next) {
				assert(iter->offset + iter->len <= iterend);
				memcpy(buf + iter->offset, iter->buf, iter->len);
			}
			ipv4_dispatch(buf, iterend, inc->h);
			free(buf);
			fragmented_free(inc);
		}
	}
}

static void fragmented_free(struct fragmented *inc) {
	if (inc->next) {
		inc->next->prev = inc->prev;
		*inc->next->prev = inc->next;
	} else {
		*inc->prev = NULL;
	}

	for (struct fragment *next, *iter = inc->first; iter; iter = next) {
		next = iter->next;
		free(iter);
	}
	free((void*)inc->h.header);
	free(inc);
}


static void ipv4_dispatch(const uint8_t *buf, size_t len, struct ipv4 ip) {
	switch (ip.proto) {
		case 0x01: icmp_parse(buf, len, ip); break;
		case 0x11:  udp_parse(buf, len, ip); break;
	}
}

void ipv4_parse(const uint8_t *buf, size_t len, struct ethernet ether) {
	uint8_t version, headerlen;
	uint16_t totallen;

	version   = buf[Version] >> 4;
	if (version != 4) return;
	headerlen = (buf[HdrLen] & 0xf) * 4;
	totallen = nget16(buf + TotalLen);
	if (totallen < headerlen) return;

	/* ignores checksum. TODO? */

	struct ipv4 ip = (struct ipv4){
		.e = ether,
		.src = nget32(buf + SrcIP),
		.dst = nget32(buf + DstIP),
		.id = nget16(buf + Id),
		.fraginfo = nget16(buf + FragInfo),
		.proto = buf[Proto],
		.header = buf,
		.hlen = headerlen,
	};

	if (ip.fraginfo & ~(EvilBit | DontFrag)) {
		fragmented_tryinsert(buf + headerlen, totallen - headerlen, ip);
	} else {
		if (totallen > len) return;
		ipv4_dispatch(buf + headerlen, totallen - headerlen, ip);
	}
}

static uint16_t next_id = 0;
void ipv4_send(const void *payload, size_t len, struct ipv4 ip) {
	const size_t mtu = 1500;
	const size_t hdrlen = 20;

	ip.e.type = ET_IPv4;
	if (!ip.src) ip.src = state.ip;
	if (!ip.e.dst && ip.dst == 0xFFFFFFFF)
		ip.e.dst = &MAC_BROADCAST;

	uint16_t id = next_id++;
	for (size_t off = 0, fraglen = mtu - hdrlen; off < len; off += fraglen) {
		if (fraglen > len - off)
			fraglen = len - off;
		bool last = off + fraglen >= len;
		uint8_t *pkt = ether_start(hdrlen + fraglen, ip.e);
		pkt[Version] = 0x40;
		pkt[HdrLen] |= hdrlen / 4;
		nput16(pkt + TotalLen, hdrlen + fraglen);
		nput16(pkt + Id, id);
		nput16(pkt + FragInfo, (off >> 3) | (last ? 0 : MoreFrags));
		pkt[TTL] = 0xFF;
		pkt[Proto] = ip.proto;
		nput32(pkt + SrcIP, ip.src);
		nput32(pkt + DstIP, ip.dst);
		nput16(pkt + Checksum, ip_checksum(pkt, hdrlen));
		memcpy(pkt + hdrlen, payload + off, fraglen);
		ether_finish(pkt);
	}
}

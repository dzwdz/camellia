#include <camellia/syscalls.h>
#include "proto.h"
#include "util.h"

enum {
	DstMAC    =  0,
	SrcMAC    =  6,
	EtherType = 12,
	Payload   = 14,
};
struct ethq *ether_queue;

void ether_parse(const uint8_t *buf, size_t len) {
	struct ethernet ether = (struct ethernet){
		.src = (void*)(buf + SrcMAC),
		.dst = (void*)(buf + DstMAC),
		.type = nget16(buf + EtherType),
	};

	for (struct ethq **iter = &ether_queue; iter && *iter; ) {
		struct ethq *qe = *iter;
		_syscall_fs_respond(qe->h, buf, len, 0);
		/* remove entry */
		/* yes, doing it this way here doesn't make sense. i'm preparing for filtering */
		*iter = qe->next;
		free(qe);
	}

	switch (ether.type) {
		case ET_IPv4:
			ipv4_parse(buf + Payload, len - Payload, ether);
			break;
		case ET_ARP:
			arp_parse(buf + Payload, len - Payload);
			break;
	}
}

static const size_t fhoff = sizeof(size_t);
uint8_t *ether_start(size_t len, struct ethernet ether) {
	if (len < 60 - Payload) len = 60 - Payload;

	if (!ether.dst) eprintf("NULL ether.dst!"); // TODO arp? i guess?
	if (!ether.src) ether.src = &state.mac;

	uint8_t *buf = malloc(fhoff + Payload + len);
	memset(buf, 0, fhoff + Payload + len);
	*(size_t*)buf = len + Payload;
	memcpy(buf + fhoff + DstMAC, ether.dst, 6);
	memcpy(buf + fhoff + SrcMAC, ether.src, 6);
	nput16(buf + fhoff + EtherType, ether.type);
	return buf + fhoff + Payload;
}
void ether_finish(uint8_t *pkt) {
	uint8_t *buf = pkt - Payload - fhoff;
	size_t len = *(size_t*)buf;
	_syscall_write(state.raw_h, buf + fhoff, len, 0, 0);
	free(buf);
}

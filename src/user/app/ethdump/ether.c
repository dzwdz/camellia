#include <camellia/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include "proto.h"
#include "util.h"

enum {
	DstMAC    =  0,
	SrcMAC    =  6,
	EtherType = 12,
	Payload   = 14,
};

void ether_parse(const uint8_t *buf, size_t len) {
	uint8_t dmac[6], smac[6];

	if (len < 60) return;
	for (int i = 0; i < 6; i++) dmac[i] = buf[i + DstMAC];
	for (int i = 0; i < 6; i++) smac[i] = buf[i + SrcMAC];
	printf("from %02x:%02x:%02x:%02x:%02x:%02x\n",
		smac[0], smac[1], smac[2], smac[3], smac[4], smac[5]);
	printf("to   %02x:%02x:%02x:%02x:%02x:%02x\n",
		dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

	uint16_t ethertype = nget16(buf + EtherType);
	printf("ethertype %u\n", ethertype);

	struct ethernet ether = (struct ethernet){
		.src = &smac,
		.dst = &dmac,
		.type = ethertype,
	};

	switch (ethertype) {
		case ET_IPv4:
			ipv4_parse(buf + Payload, len - Payload, ether);
			break;
		case ET_ARP:
			arp_parse(buf + Payload, len - Payload);
			break;
		default:
			printf("(unknown)\n");
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
	printf("sending:\n");
	hexdump(buf + fhoff, len);
	_syscall_write(state.raw_h, buf + fhoff, len, 0, 0);
	free(buf);
}

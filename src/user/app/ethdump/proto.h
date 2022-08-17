#pragma once
#include <camellia/types.h>
#include <stdint.h>

typedef uint8_t mac_t[6];

extern struct net_state {
	mac_t mac;
	uint32_t ip;

	handle_t raw_h;
} state;

enum { /* ethertype */
	ET_IPv4 = 0x800,
	ET_ARP  = 0x806,
};

struct ethernet {
	const mac_t *src, *dst;
	uint16_t type;
};


void arp_parse(const uint8_t *buf, size_t len);
void icmp_parse(const uint8_t *buf, size_t len);
void ipv4_parse(const uint8_t *buf, size_t len);

void ether_parse(const uint8_t *buf, size_t len);
uint8_t *ether_start(size_t len, struct ethernet ether);
void ether_finish(uint8_t *pkt);

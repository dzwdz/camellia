#pragma once
#include <camellia/types.h>
#include <stdint.h>

typedef uint8_t mac_t[6];
static const mac_t MAC_BROADCAST = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

struct ipv4 {
	struct ethernet e;
	uint32_t src, dst;
	uint8_t proto;
};

struct icmp {
	struct ipv4 ip;
	uint8_t type, code;
};


/* NOT THREADSAFE, YET USED FROM THREADS
 * will break if i implement a scheduler*/
struct queue_entry {
	handle_t h;
	struct queue_entry *next;
};
extern struct queue_entry *ether_queue;


void arp_parse(const uint8_t *buf, size_t len);

void icmp_parse(const uint8_t *buf, size_t len, struct ipv4 ip);
uint8_t *icmp_start(size_t len, struct icmp i);
void icmp_finish(uint8_t *pkt);

void ipv4_parse(const uint8_t *buf, size_t len, struct ethernet ether);
uint8_t *ipv4_start(size_t len, struct ipv4 ip);
void ipv4_finish(uint8_t *pkt);

void ether_parse(const uint8_t *buf, size_t len);
uint8_t *ether_start(size_t len, struct ethernet ether);
void ether_finish(uint8_t *pkt);

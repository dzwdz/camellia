#pragma once
#include <camellia/types.h>
#include <stdbool.h>
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
	uint16_t id, fraginfo;
	uint8_t proto;
	const uint8_t *header; size_t hlen;
};

struct tcp {
	struct ipv4 ip;
	uint16_t src, dst;
};

struct udp {
	struct ipv4 ip;
	uint16_t src, dst;
};

struct icmp {
	struct ipv4 ip;
	uint8_t type, code;
};


/* NOT THREADSAFE, YET USED FROM CONCURRENT THREADS
 * will break if i implement a scheduler*/
struct ethq {
	struct ethq *next;
	handle_t h;
};
extern struct ethq *ether_queue;

void arp_parse(const uint8_t *buf, size_t len);
/* 0 on success, -1 on failure */
int arpcache_get(uint32_t ip, mac_t *mac);
void arp_fsread(handle_t h, long offset);

void icmp_parse(const uint8_t *buf, size_t len, struct ipv4 ip);
void icmp_send(const void *payload, size_t len, struct icmp i);

void ipv4_parse(const uint8_t *buf, size_t len, struct ethernet ether);
void ipv4_send(const void *payload, size_t len, struct ipv4 ip);

void ether_parse(const uint8_t *buf, size_t len);
uint8_t *ether_start(size_t len, struct ethernet ether);
void ether_finish(uint8_t *pkt);

struct udp_conn;
void udp_parse(const uint8_t *buf, size_t len, struct ipv4 ip);
/* calls callback once, after a client connects. */
void udp_listen(
	uint16_t port,
	void (*on_conn)(struct udp_conn *, void *carg),
	void (*on_recv)(const void *, size_t, void *carg),
	void *carg);
struct udp_conn *udpc_new(
	struct udp u,
	void (*on_recv)(const void *, size_t, void *carg),
	void *carg);
// TODO udp_onclosed
void udpc_send(struct udp_conn *, const void *buf, size_t len);
/* frees */
void udpc_close(struct udp_conn *);

struct tcp_conn;
void tcp_parse(const uint8_t *buf, size_t len, struct ipv4 ip);
void tcp_listen(
	uint16_t port,
	void (*on_conn)(struct tcp_conn *, void *carg),
	void (*on_recv)(void *carg),
	void (*on_close)(void *carg),
	void *carg);
size_t tcpc_tryread(struct tcp_conn *, void *buf, size_t len);
void tcpc_close(struct tcp_conn *);

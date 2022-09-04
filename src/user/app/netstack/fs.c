/*
 * path format:
 * /net/raw
 *   raw ethernet frames (read-write)
 * /net/arp
 *   ARP cache (currently read-only)
 * /net/connect/0.0.0.0/1.2.3.4/udp/53
 *   connect from 0.0.0.0 (any ip) to 1.2.3.4 on udp port 53
 * /net/listen/0.0.0.0/{tcp,udp}/53
 *   waits for a connection to any ip on udp port 53
 *   open() returns once a connection to ip 0.0.0.0 on udp port 53 is received
 */
#include "proto.h"
#include "util.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum handle_type {
	H_ETHER,
	H_TCP,
	H_UDP,
	H_ARP,
};

struct strqueue {
	struct strqueue *next;
	size_t len;
	char buf[];
};

struct handle {
	enum handle_type type;
	struct {
		struct udp_conn *c;
		struct strqueue *rx, *rxlast;
	} udp;
	struct {
		struct tcp_conn *c;
		size_t readcap;
	} tcp;
	bool dead;
	handle_t reqh;
};


static void tcp_listen_callback(struct tcp_conn *c, void *arg) {
	struct handle *h = arg;
	h->tcp.c = c;
	_syscall_fs_respond(h->reqh, h, 0, 0);
	h->reqh = -1;
}

/* also called from recv_enqueue. yes, it's a mess */
static void tcp_recv_callback(void *arg) {
	struct handle *h = arg;
	char buf[1024];
	if (h->reqh >= 0) {
		if (h->tcp.readcap > sizeof buf)
			h->tcp.readcap = sizeof buf;
		size_t len = tcpc_tryread(h->tcp.c, buf, h->tcp.readcap);
		if (len > 0) {
			_syscall_fs_respond(h->reqh, buf, len, 0);
			h->reqh = -1;
		}
	}
}

static void tcp_close_callback(void *arg) {
	struct handle *h = arg;
	h->dead = true;
	if (h->reqh >= 0) {
		_syscall_fs_respond(h->reqh, NULL, -ECONNRESET, 0);
		h->reqh = -1;
		return;
	}
}

static void udp_listen_callback(struct udp_conn *c, void *arg) {
	struct handle *h = arg;
	h->udp.c = c;
	_syscall_fs_respond(h->reqh, h, 0, 0);
	h->reqh = -1;
}

static void udp_recv_callback(const void *buf, size_t len, void *arg) {
	struct handle *h = arg;
	if (h->reqh >= 0) {
		_syscall_fs_respond(h->reqh, buf, len, 0);
		h->reqh = -1;
		return;
	}
	struct strqueue *sq = malloc(sizeof(*sq) + len);
	sq->next = NULL;
	sq->len = len;
	memcpy(sq->buf, buf, len);
	if (h->udp.rx) {
		h->udp.rxlast->next = sq;
		h->udp.rxlast = sq;
	} else {
		h->udp.rx = sq;
		h->udp.rxlast = sq;
	}
}

static void recv_enqueue(struct handle *h, handle_t reqh, size_t readcap) {
	if (h->reqh > 0) {
		// TODO queue
		_syscall_fs_respond(reqh, NULL, -1, 0);
		return;
	}
	if (h->type == H_UDP && h->udp.rx) {
		_syscall_fs_respond(reqh, h->udp.rx->buf, h->udp.rx->len, 0);
		h->udp.rx = h->udp.rx->next;
		free(h->udp.rx);
		return;
	}
	h->reqh = reqh;
	if (h->type == H_TCP) {
		h->tcp.readcap = readcap;
		tcp_recv_callback(h);
	}
}

static void fs_open(handle_t reqh, char *path, int flags) {
#define respond(buf, val) do{ _syscall_fs_respond(reqh, buf, val, 0); return; }while(0)
	struct handle *h;
	if (*path != '/') respond(NULL, -1);
	path++;

	if (strcmp(path, "raw") == 0) {
		h = malloc(sizeof *h);
		memset(h, 0, sizeof *h);
		h->type = H_ETHER;
		respond(h, 0);
	} else if (strcmp(path, "arp") == 0) {
		h = malloc(sizeof *h);
		memset(h, 0, sizeof *h);
		h->type = H_ARP;
		respond(h, 0);
	}

	/* everything below ends up sending packets */
	if (flags & OPEN_RO) respond(NULL, -EACCES);

	char *save;
	const char *verb, *proto, *port_s;
	uint32_t srcip, dstip;

	verb = strtok_r(path, "/", &save);
	if (!verb) respond(NULL, -1);

	if (ip_parse(strtok_r(NULL, "/", &save), &srcip) < 0)
		respond(NULL, -1);
	if (srcip != 0) {
		eprintf("unimplemented");
		respond(NULL, -1);
	}

	if (strcmp(verb, "listen") == 0) {
		proto = strtok_r(NULL, "/", &save);
		if (!proto) respond(NULL, -1);
		if (strcmp(proto, "udp") == 0) {
			port_s = strtok_r(NULL, "/", &save);
			if (port_s) {
				uint16_t port = strtol(port_s, NULL, 0);
				h = malloc(sizeof *h);
				memset(h, 0, sizeof *h);
				h->type = H_UDP;
				h->reqh = reqh;
				udp_listen(port, udp_listen_callback, udp_recv_callback, h);
				return;
			}
		}
		if (strcmp(proto, "tcp") == 0) {
			port_s = strtok_r(NULL, "/", &save);
			if (port_s) {
				uint16_t port = strtol(port_s, NULL, 0);
				h = malloc(sizeof *h);
				memset(h, 0, sizeof *h);
				h->type = H_TCP;
				h->reqh = reqh;
				tcp_listen(port, tcp_listen_callback, tcp_recv_callback, tcp_close_callback, h);
				return;
			}
		}
	} else if (strcmp(verb, "connect") == 0) {
		if (ip_parse(strtok_r(NULL, "/", &save), &dstip) < 0)
			respond(NULL, -1);
		proto = strtok_r(NULL, "/", &save);
		if (!proto) respond(NULL, -1);
		if (strcmp(proto, "tcp") == 0) {
			port_s = strtok_r(NULL, "/", &save);
			if (port_s) {
				uint16_t port = strtol(port_s, NULL, 0);
				h = malloc(sizeof *h);
				memset(h, 0, sizeof *h);
				h->type = H_TCP;
				h->tcp.c = tcpc_new((struct tcp){
					.dst = port,
					.ip.dst = dstip,
				}, tcp_recv_callback, tcp_close_callback, h);
				if (h->tcp.c) {
					respond(h, 0);
				} else {
					free(h);
					respond(NULL, -1);
				}
			}
		}
		if (strcmp(proto, "udp") == 0) {
			port_s = strtok_r(NULL, "/", &save);
			if (port_s) {
				uint16_t port = strtol(port_s, NULL, 0);
				h = malloc(sizeof *h);
				memset(h, 0, sizeof *h);
				h->type = H_UDP;
				h->udp.c = udpc_new((struct udp){
					.dst = port,
					.ip.dst = dstip,
				}, udp_recv_callback, h);
				if (h->udp.c) {
					respond(h, 0);
				} else {
					free(h);
					respond(NULL, -1);
				}
			}
		}
	}
	respond(NULL, -1);
#undef respond
}

void fs_thread(void *arg) { (void)arg;
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		struct fs_wait_response res;
		handle_t reqh = _syscall_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;
		struct handle *h = res.id;
		long ret;
		switch (res.op) {
			case VFSOP_OPEN:
				if (res.len < buflen) {
					buf[res.len] = '\0';
					fs_open(reqh, buf, res.flags);
				} else {
					_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_READ:
				if (h->dead) {
					_syscall_fs_respond(reqh, NULL, -ECONNRESET, 0);
					break;
				}
				switch (h->type) {
					case H_ETHER: {
						struct ethq *qe;
						qe = malloc(sizeof *qe);
						qe->h = reqh;
						qe->next = ether_queue;
						ether_queue = qe;
						break;}
					case H_TCP:
					case H_UDP:
						recv_enqueue(h, reqh, res.capacity);
						break;
					case H_ARP:
						arp_fsread(reqh, res.offset);
						break;
					default:
						_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_WRITE:
				if (h->dead) {
					_syscall_fs_respond(reqh, NULL, -ECONNRESET, 0);
					break;
				}
				switch (h->type) {
					case H_ETHER:
						ret = _syscall_write(state.raw_h, buf, res.len, 0, 0);
						_syscall_fs_respond(reqh, NULL, ret, 0);
						break;
					case H_TCP:
						tcpc_send(h->tcp.c, buf, res.len);
						_syscall_fs_respond(reqh, NULL, res.len, 0);
						break;
					case H_UDP:
						udpc_send(h->udp.c, buf, res.len);
						_syscall_fs_respond(reqh, NULL, res.len, 0);
						break;
					case H_ARP:
						_syscall_fs_respond(reqh, NULL, arp_fswrite(buf, res.len, res.offset), 0);
						break;
					default:
						_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_CLOSE:
				// TODO remove entries in queue
				// TODO why does close even have _syscall_fs_respond?
				if (h->type == H_TCP) tcpc_close(h->tcp.c);
				if (h->type == H_UDP) udpc_close(h->udp.c);
				free(h);
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	free(buf);
}

#include "proto.h"
#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/fs/dir.h>

enum handle_type {
	H_ROOT,
	H_ETHER,
	H_UDP,
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
	handle_t reqh;
};


static void udp_listen_callback(struct udp_conn *c, void *arg) {
	struct handle *h = arg;
	h->udp.c = c;
	h->udp.rx = NULL;
	h->udp.rxlast = NULL;
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
	// TODO don't malloc on the network thread, dumbass
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

static void udp_recv_enqueue(struct handle *h, handle_t reqh) {
	if (h->reqh > 0) {
		// TODO queue
		_syscall_fs_respond(reqh, NULL, -1, 0);
	} else if (h->udp.rx) {
		_syscall_fs_respond(reqh, h->udp.rx->buf, h->udp.rx->len, 0);
		h->udp.rx = h->udp.rx->next;
		free(h->udp.rx);
	} else {
		h->reqh = reqh;
	}
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
				if (res.len == 1 && !memcmp("/", buf, 1)) {
					h = malloc(sizeof *h);
					h->type = H_ROOT;
					_syscall_fs_respond(reqh, h, 0, 0);
				} else if (res.len == 4 && !memcmp("/raw", buf, 4)) {
					h = malloc(sizeof *h);
					h->type = H_ETHER;
					_syscall_fs_respond(reqh, h, 0, 0);
				} else if (res.len > 6 && !memcmp("/udpl/", buf, 6)) {
					uint16_t port = strtol(buf + 6, NULL, 0);
					h = malloc(sizeof *h);
					h->type = H_UDP;
					h->udp.c = NULL;
					h->reqh = reqh;
					udp_listen(port, udp_listen_callback, udp_recv_callback, h);
				} else {
					_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_READ:
				switch (h->type) {
					case H_ROOT: {
						struct dirbuild db;
						dir_start(&db, res.offset, buf, sizeof buf);
						dir_append(&db, "raw");
						dir_append(&db, "udpl/");
						_syscall_fs_respond(reqh, buf, dir_finish(&db), 0);
						break;}
					case H_ETHER: {
						struct ethq *qe;
						qe = malloc(sizeof *qe);
						qe->h = reqh;
						qe->next = ether_queue;
						ether_queue = qe;
						break;}
					case H_UDP:
						udp_recv_enqueue(h, reqh);
						break;
					default:
						_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_WRITE:
				switch (h->type) {
					case H_ETHER:
						ret = _syscall_write(state.raw_h, buf, res.len, 0, 0);
						_syscall_fs_respond(reqh, NULL, ret, 0);
						break;
					case H_UDP:
						udpc_send(h->udp.c, buf, res.len);
						_syscall_fs_respond(reqh, NULL, res.len, 0);
						break;
					default:
						_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;
			case VFSOP_CLOSE:
				// TODO remove entries in queue
				// TODO why does close even have _syscall_fs_respond?
				if (h->type == H_UDP)
					udpc_close(h->udp.c);
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

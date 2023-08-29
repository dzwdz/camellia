#include "proto.h"
#include "util.h"
#include <camellia.h>
#include <camellia/compat.h>
#include <camellia/syscalls.h>
#include <err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>

struct net_state state = {
	// TODO dynamically get mac
	.mac = {0x52, 0x54, 0x00, 0xCA, 0x77, 0x1A},
};

void network_thread(void *arg) { (void)arg;
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _sys_read(state.raw_h, buf, buflen, -1);
		if (ret < 0) break;
		ether_parse((void*)buf, ret);
	}
	free(buf);
}

void fs_thread(void *arg);

int main(int argc, char **argv) {
	if (argc < 4) {
		fprintf(stderr, "usage: netstack iface ip gateway\n");
		return 1;
	}
	state.raw_h = camellia_open(argv[1], OPEN_RW);
	if (state.raw_h < 0) {
		err(1, "open %s", argv[1]);
		return 1;
	}
	if (ip_parse(argv[2], &state.ip) < 0) {
		errx(1, "invalid ip: %s", argv[2]);
		return -1;
	}
	if (ip_parse(argv[3], &state.gateway) < 0) {
		errx(1, "invalid gateway: %s", argv[2]);
		return -1;
	}
	setproctitle(argv[2]);
	arp_request(state.gateway);
	thread_create(0, network_thread, NULL);
	thread_create(0, fs_thread, NULL);
	_sys_await();
	return 0;
}

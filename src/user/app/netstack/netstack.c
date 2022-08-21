#include "proto.h"
#include "util.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/thread.h>

struct net_state state = {
	// TODO dynamically get mac
	.mac = {0x52, 0x54, 0x00, 0xCA, 0x77, 0x1A},
	.ip = (192 << 24) + (168 << 16) + 11,
};

void network_thread(void *arg) { (void)arg;
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _syscall_read(state.raw_h, buf, buflen, -1);
		if (ret < 0) break;
		ether_parse((void*)buf, ret);
	}
	free(buf);
}

void fs_thread(void *arg);

int main(int argc, char **argv) {
	if (argc < 2) {
		eprintf("no argument");
		return 1;
	}
	state.raw_h = _syscall_open(argv[1], strlen(argv[1]), 0);
	if (state.raw_h < 0) {
		eprintf("couldn't open %s", argv[1]);
		return 1;
	}
	thread_create(0, network_thread, NULL);
	thread_create(0, fs_thread, NULL);
	_syscall_await();
	return 0;
}

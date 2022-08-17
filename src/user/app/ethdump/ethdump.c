#include "proto.h"
#include "util.h"
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct net_state state = {
	// TODO dynamically get mac
	.mac = {0x52, 0x54, 0x00, 0xCA, 0x77, 0x1A},
	.ip = (192 << 24) + (168 << 16) + 11,
};

int main(void) {
	const char *path = "/kdev/eth";
	state.raw_h = _syscall_open(path, strlen(path), 0);
	if (state.raw_h < 0) {
		eprintf("couldn't open %s", path);
		return 1;
	}

	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _syscall_read(state.raw_h, buf, buflen, -1);
		if (ret < 0) break;
		printf("\npacket of length %u\n", ret);
		hexdump(buf, ret);
		ether_parse((void*)buf, ret);
	}
	free(buf);
	return 0;
}

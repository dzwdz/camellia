#include "proto.h"
#include "util.h"

void icmp_parse(const uint8_t *buf, size_t len) {
	if (len < 4) return;
	uint8_t type = buf[0];
	printf("ICMP type %u\n", type);
}

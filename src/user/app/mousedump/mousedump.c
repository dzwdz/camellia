#include <camellia/syscalls.h>
#include <unistd.h>
#include <shared/container/ring.h>
#include <stdio.h>

static uint8_t r_buf[64];
static ring_t r = {(void*)r_buf, sizeof r_buf, 0, 0};

struct packet {
	uint8_t stuff;
	int8_t dx, dy;
} __attribute__((packed));

int main(void) {
	char buf[64];
	handle_t fd = _syscall_open("/kdev/ps2/mouse", 15, 0);
	if (fd < 0) {
		fprintf(stderr, "couldn't open mouse\n");
		exit(1);
	}
	for (;;) {
		int len = _syscall_read(fd, buf, sizeof buf, 0);
		if (len == 0) break;
		ring_put(&r, buf, len);
		while (ring_size(&r) >= 3) {
			struct packet p;
			ring_get(&r, &p, sizeof p);
			printf("%4d %4d\n", p.dx, p.dy);
		}
	}
	return 0;
}

#include "../tests.h"
#include <shared/container/ring.h>
#include <string.h>

static void test_ringbuf(void) {
	char backbuf[16], cmpbuf[16];
	size_t num_read = 0, num_written = 0;
	uint8_t c;

	ring_t r = {backbuf, 16, 0, 0};

	/* aliasing */
	for (size_t i = 0; i < 16; i++) {
		test(ring_used(&r) == 0);
		test(ring_avail(&r) == 16);
		ring_put(&r, "11 bytes...", 11);
		test(ring_used(&r) == 11);
		test(ring_avail(&r) == 5);

		memset(cmpbuf, 0, sizeof cmpbuf);
		test(ring_get(&r, cmpbuf, 16) == 11);
		test(memcmp(cmpbuf, "11 bytes...", 11) == 0);
	}

	test(ring_used(&r) == 0);
	for (size_t i = 0; i < 7; i++)
		ring_put1b(&r, num_written++);
	test(ring_used(&r) == 7);
	for (size_t i = 0; i < 3; i++) {
		ring_get(&r, &c, 1);
		test(num_read++ == c);
	}
	test(ring_used(&r) == 4);

	for (size_t j = 0; j < 40; j++) {
		for (size_t i = 0; i < 7; i++)
			ring_put1b(&r, num_written++ & 0xff);
		test(ring_used(&r) == 11);
		for (size_t i = 0; i < 7; i++) {
			ring_get(&r, &c, 1);
			test((num_read++ & 0xff) == c);
		}
		test(ring_used(&r) == 4);
	}
}

void r_s_ringbuf(void) {
	run_test(test_ringbuf);
}

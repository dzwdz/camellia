#include <kernel/tests/base.h>
#include <kernel/tests/tests.h>
#include <shared/container/ring.h>
#include <shared/mem.h>

TEST(memcmp) {
	// basic equality checks
	TEST_COND(0 == memcmp("some", "thing", 0));
	TEST_COND(0 != memcmp("some", "thing", 1));
	TEST_COND(0 != memcmp("some", "thing", 4));

	TEST_COND(0 == memcmp("test", "tennis",  0));
	TEST_COND(0 == memcmp("test", "tennis",  1));
	TEST_COND(0 == memcmp("test", "tennis",  2));
	TEST_COND(0 != memcmp("test", "tennis",  3));
	TEST_COND(0 != memcmp("test", "tennis",  4));
	TEST_COND(0 != memcmp("test", "tennis",  5));

	// test signs. does anyone even use that?
	TEST_COND(0 > memcmp("foo", "moo", 4));
	TEST_COND(0 < memcmp("moo", "foo", 4));
	TEST_COND(0 > memcmp("555", "654", 3));
	TEST_COND(0 < memcmp("654", "555", 3));
}

TEST(strcmp) {
	TEST_COND(0 == strcmp("string", "string"));
	TEST_COND(0 > strcmp("str", "string"));
	TEST_COND(0 < strcmp("string", "str"));

	TEST_COND(0 != strcmp("stress", "string"));
}

TEST(ring) {
	char backbuf[16];
	size_t num_read = 0, num_written = 0;
	uint8_t c;

	ring_t r = {backbuf, 16, 0, 0};

	TEST_COND(ring_size(&r) == 0);
	for (size_t i = 0; i < 7; i++)
		ring_put1b(&r, num_written++);
	TEST_COND(ring_size(&r) == 7);
	for (size_t i = 0; i < 3; i++) {
		ring_get(&r, &c, 1);
		TEST_COND(num_read++ == c);
	}
	TEST_COND(ring_size(&r) == 4);

	for (size_t j = 0; j < 40; j++) {
		for (size_t i = 0; i < 7; i++)
			ring_put1b(&r, num_written++ & 0xff);
		TEST_COND(ring_size(&r) == 11);
		for (size_t i = 0; i < 7; i++) {
			ring_get(&r, &c, 1);
			TEST_COND((num_read++ & 0xff) == c);
		}
		TEST_COND(ring_size(&r) == 4);
	}
}

void tests_utils(void) {
	TEST_RUN(memcmp);
	TEST_RUN(strcmp);
	TEST_RUN(ring);
}

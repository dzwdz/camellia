#include <kernel/tests/base.h>
#include <kernel/util.h>

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

void tests_utils(void) {
	TEST_RUN(memcmp);
}

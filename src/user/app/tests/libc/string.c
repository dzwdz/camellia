#include "../tests.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void test_memcmp(void) {
	test(0 == memcmp("some", "thing", 0));
	test(0 != memcmp("some", "thing", 1));
	test(0 != memcmp("some", "thing", 4));

	test(0 == memcmp("test", "tennis",  0));
	test(0 == memcmp("test", "tennis",  1));
	test(0 == memcmp("test", "tennis",  2));
	test(0 != memcmp("test", "tennis",  3));
	test(0 != memcmp("test", "tennis",  4));
	test(0 != memcmp("test", "tennis",  5));

	test(0 > memcmp("foo", "moo", 4));
	test(0 < memcmp("moo", "foo", 4));
	test(0 > memcmp("555", "654", 3));
	test(0 < memcmp("654", "555", 3));
}

static bool memall(const unsigned char *s, unsigned char c, size_t n) {
	for (size_t i = 0; i < n; i++)
		if (s[i] != c) return false;
	return true;
}

static void test_memset(void) {
	const size_t buflen = 4096;
	void *buf = malloc(buflen);
	test(buf);
	for (int i = 0; i < 257; i++) {
		memset(buf, i, buflen);
		test(memall(buf, i & 0xff, buflen));
	}
	free(buf);
}

static void test_strcmp(void) {
	test(0 == strcmp("string", "string"));
	test(0 > strcmp("str", "string"));
	test(0 < strcmp("string", "str"));

	test(0 != strcmp("stress", "string"));
}

static void test_strtol(void) {
	char *end;
	test(1234 == strtol("1234", NULL, 10));
	test(1234 == strtol("+1234", NULL, 10));
	test(-1234 == strtol("-1234", NULL, 10));

	test(1234 == strtol("1234", &end, 10));
	test(!strcmp("", end));
	test(1234 == strtol("   1234hello", &end, 10));
	test(!strcmp("hello", end));

	test(1234 == strtol("   1234hello", &end, 0));
	test(!strcmp("hello", end));
	test(0xCAF3 == strtol("   0xCaF3hello", &end, 0));
	test(!strcmp("hello", end));
	test(01234 == strtol("   01234hello", &end, 0));
	test(!strcmp("hello", end));
}

void r_libc_string(void) {
	run_test(test_memcmp);
	run_test(test_memset);
	run_test(test_strcmp);
	run_test(test_strtol);
}

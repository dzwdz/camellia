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

static void test_memmove(void) {
	const int partsize = 64;
	char buf[partsize * 3];
	for (int i = 0; i < partsize * 2; i++) {
		memset(buf, 0, sizeof buf);
		for (int j = 0; j < partsize; j++) buf[i + j] = j;
		memmove(buf + partsize, buf + i, partsize);
		for (int j = 0; j < partsize; j++) test(buf[partsize + j] == j);
	}
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

static void test_strspn(void) {
	test(0 == strspn("", "1234"));
	test(0 == strspn("asdf", "1234"));
	test(0 == strspn("a2df", "1234"));
	test(2 == strspn("42df", "1234"));
	test(4 == strspn("4211", "1234"));

	test(0 == strcspn("", "1234"));
	test(4 == strcspn("asdf", "1234"));
	test(1 == strcspn("a2df", "1234"));
}

static void test_strtok(void) {
	const char *sep = " \t";
	{
		char line[] = "LINE TO BE SEPARATED";
		test(!strcmp(strtok(line, sep), "LINE"));
		test(!strcmp(strtok(NULL, sep), "TO"));
		test(!strcmp(strtok(NULL, sep), "BE"));
		test(!strcmp(strtok(NULL, sep), "SEPARATED"));
		for (int i = 0; i < 4; i++)
			test(strtok(NULL, sep) == NULL);
	}
	{
		char line[] = "  LINE TO\tBE  \t SEPARATED   ";
		test(!strcmp(strtok(line, sep), "LINE"));
		test(!strcmp(strtok(NULL, sep), "TO"));
		test(!strcmp(strtok(NULL, sep), "BE"));
		test(!strcmp(strtok(NULL, sep), "SEPARATED"));
		for (int i = 0; i < 4; i++)
			test(strtok(NULL, sep) == NULL);
	}
}

void r_libc_string(void) {
	run_test(test_memcmp);
	run_test(test_memset);
	run_test(test_memmove);
	run_test(test_strcmp);
	run_test(test_strtol);
	run_test(test_strspn);
	run_test(test_strtok);
}

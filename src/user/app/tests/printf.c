#define TEST_MACROS
#include "tests.h"
#include <stdio.h>
#include <string.h>

void test_printf(void) {
	char buf[64];
	memset(buf, '?', 64);

	/* test proper overflow handling in snprintf */
	assert(13 == snprintf(buf, 15, "That's 0x%x", 0x1337));
	assert(!memcmp(buf, "That's 0x1337\0??", 16));
	assert(17 == snprintf(buf, 15, "%05x %05x %05x", 0, 0, 0));
	assert(!memcmp(buf, "00000 00000 00\0?", 16));

	/* all the other stuff */
	snprintf(buf, sizeof buf, "%010x", 0x1BABE);
	assert(!strcmp(buf, "000001babe"));
	snprintf(buf, sizeof buf, "%10x", 0x1BABE);
	assert(!strcmp(buf, "     1babe"));
	snprintf(buf, sizeof buf, "%10s", "hello");
	assert(!strcmp(buf, "     hello"));

	snprintf(buf, sizeof buf, "%s%%%s", "ab", "cd");
	assert(!strcmp(buf, "ab%cd"));

	snprintf(buf, sizeof buf, "%05u %05u", 1234, 56789);
	assert(!strcmp(buf, "01234 56789"));

	snprintf(buf, sizeof buf, "%u %x", 0, 0);
	assert(!strcmp(buf, "0 0"));
}

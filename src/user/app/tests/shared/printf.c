#include "../tests.h"
#include <stdio.h>
#include <string.h>

static void test_printf(void) {
	char buf[64];
	memset(buf, '?', 64);

	/* test proper overflow handling in snprintf */
	test(13 == snprintf(buf, 15, "That's 0x%x", 0x1337));
	test(!memcmp(buf, "That's 0x1337\0??", 16));
	test(17 == snprintf(buf, 15, "%05x %05x %05x", 0, 0, 0));
	test(!memcmp(buf, "00000 00000 00\0?", 16));

	/* all the other stuff */
	snprintf(buf, sizeof buf, "%010x", 0x1BABE);
	test(!strcmp(buf, "000001babe"));
	snprintf(buf, sizeof buf, "%10x", 0x1BABE);
	test(!strcmp(buf, "     1babe"));
	snprintf(buf, sizeof buf, "%10s", "hello");
	test(!strcmp(buf, "     hello"));

	snprintf(buf, sizeof buf, "%s%%%s", "ab", "cd");
	test(!strcmp(buf, "ab%cd"));

	snprintf(buf, sizeof buf, "%05u %05u", 1234, 56789);
	test(!strcmp(buf, "01234 56789"));

	snprintf(buf, sizeof buf, "%5d %5d", 123, 4567);
	test(!strcmp(buf, "  123  4567"));
	snprintf(buf, sizeof buf, "%5d %5d", -123, -4567);
	test(!strcmp(buf, " -123 -4567"));

	snprintf(buf, sizeof buf, "%u %d %x", 0, 0, 0);
	test(!strcmp(buf, "0 0 0"));
}

void r_s_printf(void) {
	run_test(test_printf);
}

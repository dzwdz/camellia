#include "../tests.h"
#include <string.h>

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
	run_test(test_strtol);
}

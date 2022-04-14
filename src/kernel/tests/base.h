#pragma once
#include <kernel/arch/generic.h>
#include <kernel/util.h>
#include <stdbool.h>

extern bool _did_tests_fail;

// tests MUST be used
#pragma GCC diagnostic error "-Wunused-function"

#define TEST(name) \
	static void __test_##name(void)

#define TEST_RUN(name) \
	__test_##name();

#define TEST_COND(cond)                           \
	if (!(cond)) {                                \
		_did_tests_fail = true;                   \
		kprintf("test failed at " __FILE__ ":" NUM2STR(__LINE__) " "); \
	}

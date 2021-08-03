#include <kernel/panic.h>
#include <kernel/tests/base.h>
#include <kernel/tests/tests.h>

bool _did_tests_fail;

TEST(basic_math) {
	TEST_IF(2 + 2 == 4);
	TEST_IF(2 * 2 == 4);
}

void tests_all() {
	_did_tests_fail = false;

	TEST_RUN(basic_math);
	tests_utils();

	if (_did_tests_fail)
		panic();
}

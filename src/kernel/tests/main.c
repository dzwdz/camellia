#include <kernel/panic.h>
#include <kernel/tests/base.h>
#include <kernel/tests/tests.h>

bool _did_tests_fail;

void tests_all() {
	_did_tests_fail = false;

	tests_utils();

	if (_did_tests_fail)
		panic();
}

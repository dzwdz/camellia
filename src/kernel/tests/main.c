#include <kernel/panic.h>
#include <kernel/tests/base.h>
#include <kernel/tests/tests.h>

bool _did_tests_fail;

void tests_all(void) {
	_did_tests_fail = false;

	tests_utils();
	tests_vfs();

	if (_did_tests_fail)
		panic_invalid_state();
}

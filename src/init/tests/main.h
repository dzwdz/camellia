#pragma once

void test_all(void);
void test_pipe(void);

#ifdef TEST_MACROS

#define argify(str) str, sizeof(str) - 1
#define test_fail() do { \
	printf("\033[31m" "TEST FAILED: %s:%xh\n" "\033[0m", __func__, __LINE__); \
	return; \
} while (0)
#define assert(cond) if (!(cond)) test_fail();

#endif

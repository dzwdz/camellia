#pragma once
#include <camellia/syscalls.h>
#include <stdio.h>
#include <unistd.h>

void stress_all(void);
void test_all(void);

void test_pipe(void);
void test_printf(void);
void test_semaphore(void);

#ifdef TEST_MACROS

#define argify(str) str, sizeof(str) - 1
#define test_fail() do { \
	printf("\033[31m" "TEST FAILED: %s():%u\n" "\033[0m", __func__, __LINE__); \
	exit(0); \
} while (0)
#define assert(cond) if (!(cond)) test_fail();

#endif

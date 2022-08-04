#pragma once
#include <camellia/syscalls.h>
#include <stdio.h>
#include <unistd.h>

#define TMPFILEPATH "/tmp/.test_internal"

void run_test(void (*fn)());

void r_k_fs(void);
void r_k_misc(void);
void r_k_miscsyscall(void);
void r_libc_esemaphore(void);
void r_libc_string(void);
void r_printf(void);

#define argify(str) str, sizeof(str) - 1
#define test_fail() do { \
	printf("\033[31m" "TEST FAILED: %s():%u\n" "\033[0m", __func__, __LINE__); \
	exit(0); \
} while (0)
#define test(cond) if (!(cond)) test_fail();

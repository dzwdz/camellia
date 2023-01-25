#include "../tests.h"
#include <camellia/errno.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <string.h>
#include <unistd.h>

static void test_fault_kill(void) {
	if (!fork()) { /* invalid memory access */
		asm volatile("movb $69, 0" ::: "memory");
		test_fail();
	}
	if (!fork()) { /* #GP */
		asm volatile("hlt" ::: "memory");
		test_fail();
	}

	int await_cnt = 0;
	while (_syscall_await() != ~0) await_cnt++;
	test(await_cnt == 2);
}

static void test_efault(void) {
	const char *str = "o, 16 characters";
	char str2[16];
	char *invalid = (void*)0x1000;
	handle_t h;

	memcpy(str2, str, 16);

	test((h = _syscall_open(TMPFILEPATH, strlen(TMPFILEPATH), OPEN_CREATE | OPEN_WRITE)) >= 0);
	test(_syscall_write(h, str, 16, 0, 0) == 16);
	test(_syscall_write(h, str2, 16, 0, 0) == 16);

	test(_syscall_write(h, invalid, 16, 0, 0) == -EFAULT);

	/* x64 non-canonical pointers */
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x8000000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x1000000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0100000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0010000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0001000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0000800000000000), 16, 0, 0) == -EFAULT);

	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x8000000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x1000000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0100000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0010000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0001000000000000), 16, 0, 0) == -EFAULT);
	test(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0000800000000000), 16, 0, 0) == -EFAULT);

	test(_syscall_write(h, str, 16, 0, 0) == 16);
	close(h);
}

static void test_invalid_syscall(void) {
	test(_syscall(~0, 0, 0, 0, 0, 0) < 0);
}

void r_k_misc(void) {
	run_test(test_fault_kill);
	// run_test(test_efault);
	run_test(test_invalid_syscall);
}

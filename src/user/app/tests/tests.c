#define TEST_MACROS
#include "tests.h"
#include <camellia/errno.h>
#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void run_forked(void (*fn)()) {
	if (!fork()) {
		fn();
		exit(0);
	} else {
		/* successful tests must return 0
		 * TODO add a better fail msg */
		if (_syscall_await() != 0) test_fail();
	}
}


const char *tmpfilepath = "/tmp/.test_internal";

static void test_await(void) {
	/* creates 16 child processes, each returning a different value. then checks
	 * if await() returns every value exactly once */
	int ret;
	int counts[16] = {0};

	for (int i = 0; i < 16; i++)
		if (!fork())
			exit(i);

	while ((ret = _syscall_await()) != ~0) {
		assert(0 <= ret && ret < 16);
		counts[ret]++;
	}

	for (int i = 0; i < 16; i++)
		assert(counts[i] == 1);
}

static void test_faults(void) {
	/* tests what happens when child processes fault.
	 * expected behavior: parent processes still manages to finish, and it can
	 * reap all its children */
	int await_cnt = 0;

	if (!fork()) { // invalid memory access
		asm volatile("movb $69, 0" ::: "memory");
		printf("this shouldn't happen");
		exit(-1);
	}
	if (!fork()) { // #GP
		asm volatile("hlt" ::: "memory");
		printf("this shouldn't happen");
		exit(-1);
	}

	while (_syscall_await() != ~0) await_cnt++;
	assert(await_cnt == 2);
}

static void test_interrupted_fs(void) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h)) { /* child */
		// TODO make a similar test with all 0s passed to fs_wait
		struct fs_wait_response res;
		_syscall_fs_wait(NULL, 0, &res);
		exit(0);
	} else { /* parent */
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		// the handler quits while handling that call - but this syscall should return anyways
		exit(ret < 0 ? 0 : -1);
	}
}

static void test_orphaned_fs(void) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h)) { /* child */
		exit(0);
	} else { /* parent */
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		// no handler will ever be available to handle this call - the syscall should instantly return
		exit(ret < 0 ? 0 : -1);
	}
}

static void test_memflag(void) {
	void *page = (void*)0x77777000;
	_syscall_memflag(page, 4096, MEMFLAG_PRESENT); // allocate page
	memset(page, 77, 4096); // write to it
	_syscall_memflag(page, 4096, 0); // free it

	if (!fork()) {
		memset(page, 11, 4096); // should segfault
		exit(0);
	} else {
		assert(_syscall_await() != 0); // test if the process crashed
	}

	char *pages[4];
	for (size_t i = 0; i < 4; i++) {
		pages[i] = _syscall_memflag(NULL, 4096, MEMFLAG_FINDFREE);
		printf("[test_memflag] findfree: 0x%x\n", pages[i]);

		assert(pages[i] == _syscall_memflag(NULL, 4096, MEMFLAG_FINDFREE | MEMFLAG_PRESENT));
		if (i > 0) assert(pages[i] != pages[i-1]);
		*pages[i] = 0x77;
	}

	for (size_t i = 0; i < 4; i++)
		_syscall_memflag(pages, 4096, MEMFLAG_PRESENT);

	// TODO check if reclaims
}

static void test_dup(void) {
	handle_t pipe[2];
	handle_t h1, h2;
	assert(_syscall_pipe(pipe, 0) >= 0);

	if (!fork()) {
		close(pipe[0]);

		h1 = _syscall_dup(pipe[1], -1, 0);
		assert(h1 >= 0);
		assert(h1 != pipe[1]);
		h2 = _syscall_dup(h1, -1, 0);
		assert(h2 >= 0);
		assert(h2 != pipe[1] && h2 != h1);

		_syscall_write(pipe[1], "og", 2, 0, 0);
		_syscall_write(h1, "h1", 2, 0, 0);
		_syscall_write(h2, "h2", 2, 0, 0);

		close(pipe[1]);
		_syscall_write(h1, "h1", 2, 0, 0);
		_syscall_write(h2, "h2", 2, 0, 0);

		assert(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		close(h1);
		close(h2);

		assert(_syscall_dup(pipe[1], h2, 0) == h2);
		_syscall_write(h2, "h2", 2, 0, 0);
		close(h2);

		assert(_syscall_dup(pipe[1], h1, 0) == h1);
		_syscall_write(h1, "h1", 2, 0, 0);
		close(h1);

		exit(0);
	} else {
		char buf[16];
		size_t count = 0;
		close(pipe[1]);
		while (_syscall_read(pipe[0], buf, sizeof buf, 0) >= 0)
			count++;
		assert(count == 7);
		_syscall_await();
	}


	close(pipe[0]);
}

static void test_malloc(void) {
	// not really a test
	void *p1, *p2;

	p1 = malloc(420);
	printf("p1 = 0x%x\n", p1);

	p2 = malloc(1024);
	printf("p2 = 0x%x\n", p2);
	free(p2);
	p2 = malloc(256);
	printf("p2 = 0x%x\n", p2);
	free(p2);
	p2 = malloc(4096);
	printf("p2 = 0x%x\n", p2);
	free(p2);

	free(p1);
}

static void test_efault(void) {
	const char *str = "o, 16 characters";
	char str2[16];
	char *invalid = (void*)0x1000;
	handle_t h;

	memcpy(str2, str, 16);

	assert((h = _syscall_open(tmpfilepath, strlen(tmpfilepath), OPEN_CREATE)));
	assert(_syscall_write(h, str, 16, 0, 0) == 16);
	assert(_syscall_write(h, str2, 16, 0, 0) == 16);

	assert(_syscall_write(h, invalid, 16, 0, 0) == -EFAULT);

	/* x64 canonical pointers */
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x8000000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x1000000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0100000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0010000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0001000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str ^ 0x0000800000000000), 16, 0, 0) == -EFAULT);

	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x8000000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x1000000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0100000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0010000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0001000000000000), 16, 0, 0) == -EFAULT);
	assert(_syscall_write(h, (void*)((uintptr_t)str2 ^ 0x0000800000000000), 16, 0, 0) == -EFAULT);

	assert(_syscall_write(h, str, 16, 0, 0) == 16);
	close(h);
}

static void test_execbuf(void) {
	// not really a test, TODO
	const char str1[] = "test_execbuf: string 1\n";
	const char str2[] = "test_execbuf: and 2\n";
	uint64_t buf[] = {
		EXECBUF_SYSCALL, _SYSCALL_WRITE, 1, (uintptr_t)str1, sizeof(str1) - 1, -1, 0,
		EXECBUF_SYSCALL, _SYSCALL_WRITE, 1, (uintptr_t)str2, sizeof(str2) - 1, -1, 0,
		EXECBUF_SYSCALL, _SYSCALL_EXIT, 0, 0, 0, 0, 0,
	};
	_syscall_execbuf(buf, sizeof buf);
	test_fail();
}

static void test_strtol(void) {
	char *end;
	assert(1234 == strtol("1234", NULL, 10));
	assert(1234 == strtol("+1234", NULL, 10));
	assert(-1234 == strtol("-1234", NULL, 10));

	assert(1234 == strtol("1234", &end, 10));
	assert(!strcmp("", end));
	assert(1234 == strtol("   1234hello", &end, 10));
	assert(!strcmp("hello", end));

	assert(1234 == strtol("   1234hello", &end, 0));
	assert(!strcmp("hello", end));
	assert(0xCAF3 == strtol("   0xCaF3hello", &end, 0));
	assert(!strcmp("hello", end));
	assert(01234 == strtol("   01234hello", &end, 0));
	assert(!strcmp("hello", end));
}

static void test_sleep(void) {
	// TODO yet another of those fake tests that you have to verify by hand
	if (!fork()) {
		if (!fork()) {
			_syscall_sleep(100);
			printf("1\n");
			_syscall_sleep(200);
			printf("3\n");
			_syscall_sleep(200);
			printf("5\n");
			_syscall_exit(0);
		}
		if (!fork()) {
			printf("0\n");
			_syscall_sleep(200);
			printf("2\n");
			_syscall_sleep(200);
			printf("4\n");
			/* get killed while asleep
			* a peaceful death, i suppose. */
			for (;;) _syscall_sleep(1000000000);
		}
		_syscall_await();
		_syscall_exit(0);
	}

	/* this part checks if, after killing an asleep process, other processes can still wake up */
	_syscall_sleep(600);
	printf("6\n");
}

static void test_misc(void) {
	assert(_syscall(~0, 0, 0, 0, 0, 0) < 0); /* try making an invalid syscall */
}


int main(void) {
	run_forked(test_await);
	run_forked(test_faults);
	run_forked(test_interrupted_fs);
	run_forked(test_orphaned_fs);
	run_forked(test_memflag);
	run_forked(test_dup);
	run_forked(test_malloc);
	run_forked(test_pipe);
	run_forked(test_semaphore);
	run_forked(test_efault);
	run_forked(test_execbuf);
	run_forked(test_printf);
	run_forked(test_strtol);
	run_forked(test_sleep);
	run_forked(test_misc);
	return 1;
}

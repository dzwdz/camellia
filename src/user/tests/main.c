#define TEST_MACROS
#include <user/lib/stdlib.h>
#include <user/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static void run_forked(void (*fn)()) {
	if (!_syscall_fork(0, NULL)) {
		fn();
		_syscall_exit(0);
	} else {
		/* successful tests must return 0
		 * TODO add a better fail msg */
		if (_syscall_await() != 0) test_fail();
	}
}



static void test_await(void) {
	/* creates 16 child processes, each returning a different value. then checks
	 * if await() returns every value exactly once */
	int ret;
	int counts[16] = {0};

	for (int i = 0; i < 16; i++)
		if (!_syscall_fork(0, NULL))
			_syscall_exit(i);

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

	if (!_syscall_fork(0, NULL)) { // invalid memory access
		asm volatile("movb $69, 0" ::: "memory");
		printf("this shouldn't happen");
		_syscall_exit(-1);
	}
	if (!_syscall_fork(0, NULL)) { // #GP
		asm volatile("hlt" ::: "memory");
		printf("this shouldn't happen");
		_syscall_exit(-1);
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
		_syscall_exit(0);
	} else { /* parent */
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		// the handler quits while handling that call - but this syscall should return anyways
		_syscall_exit(ret < 0 ? 0 : -1);
	}
}

static void test_orphaned_fs(void) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h)) { /* child */
		_syscall_exit(0);
	} else { /* parent */
		_syscall_mount(h, "/", 1);
		int ret = _syscall_open("/", 1, 0);
		// no handler will ever be available to handle this call - the syscall should instantly return
		_syscall_exit(ret < 0 ? 0 : -1);
	}
}

static void test_memflag(void) {
	void *page = (void*)0x77777000;
	_syscall_memflag(page, 4096, MEMFLAG_PRESENT); // allocate page
	memset(page, 77, 4096); // write to it
	_syscall_memflag(page, 4096, 0); // free it

	if (!_syscall_fork(0, NULL)) {
		memset(page, 11, 4096); // should segfault
		_syscall_exit(0);
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

	if (!_syscall_fork(0, NULL)) {
		_syscall_close(pipe[0]);

		h1 = _syscall_dup(pipe[1], -1, 0);
		assert(h1 >= 0);
		assert(h1 != pipe[1]);
		h2 = _syscall_dup(h1, -1, 0);
		assert(h2 >= 0);
		assert(h2 != pipe[1] && h2 != h1);

		_syscall_write(pipe[1], "og", 2, 0);
		_syscall_write(h1, "h1", 2, 0);
		_syscall_write(h2, "h2", 2, 0);

		_syscall_close(pipe[1]);
		_syscall_write(h1, "h1", 2, 0);
		_syscall_write(h2, "h2", 2, 0);

		assert(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		assert(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		_syscall_close(h1);
		_syscall_close(h2);

		assert(_syscall_dup(pipe[1], h2, 0) == h2);
		_syscall_write(h2, "h2", 2, 0);
		_syscall_close(h2);

		assert(_syscall_dup(pipe[1], h1, 0) == h1);
		_syscall_write(h1, "h1", 2, 0);
		_syscall_close(h1);

		_syscall_exit(0);
	} else {
		char buf[16];
		size_t count = 0;
		_syscall_close(pipe[1]);
		while (_syscall_read(pipe[0], buf, sizeof buf, 0) >= 0)
			count++;
		assert(count == 7);
		_syscall_await();
	}


	_syscall_close(pipe[0]);
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

static void test_misc(void) {
	assert(_syscall(~0, 0, 0, 0, 0) < 0); /* try making an invalid syscall */
}


void test_all(void) {
	run_forked(test_await);
	run_forked(test_faults);
	run_forked(test_interrupted_fs);
	run_forked(test_orphaned_fs);
	run_forked(test_memflag);
	run_forked(test_dup);
	run_forked(test_malloc);
	run_forked(test_pipe);
	run_forked(test_semaphore);
	run_forked(test_misc);
}

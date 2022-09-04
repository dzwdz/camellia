#include "../tests.h"
#include <camellia/errno.h>
#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <string.h>
#include <unistd.h>


static void test_await(void) {
	/* creates 16 child processes, each returning a different value. then checks
	 * if await() returns every value exactly once */
	int ret;
	int counts[16] = {0};

	for (int i = 0; i < 16; i++)
		if (!fork())
			exit(i);

	while ((ret = _syscall_await()) != ~0) {
		test(0 <= ret && ret < 16);
		counts[ret]++;
	}

	for (int i = 0; i < 16; i++)
		test(counts[i] == 1);
}

static void test_pipe(void) {
	const char *pipe_msgs[2] = {"hello", "world"};
	handle_t ends[2];
	char buf[16];

	/* test regular reads / writes */
	test(_syscall_pipe(ends, 0) >= 0);
	if (!fork()) {
		/* those repeated asserts ensure that you can't read/write to the wrong ends */
		test(_syscall_read(ends[1], buf, 16, 0) < 0);
		test(_syscall_write(ends[0], buf, 16, 0, 0) < 0);

		test(5 == _syscall_write(ends[1], pipe_msgs[0], 5, -1, 0));

		test(_syscall_read(ends[1], buf, 16, 0) < 0);
		test(_syscall_write(ends[0], buf, 16, 0, 0) < 0);

		test(5 == _syscall_write(ends[1], pipe_msgs[1], 5, -1, 0));

		exit(0);
	} else {
		test(_syscall_read(ends[1], buf, 16, 0) < 0);
		test(_syscall_write(ends[0], buf, 16, 0, 0) < 0);

		test(5 == _syscall_read(ends[0], buf, 16, 0));
		test(!memcmp(buf, pipe_msgs[0], 5));

		test(_syscall_read(ends[1], buf, 16, 0) < 0);
		test(_syscall_write(ends[0], buf, 16, 0, 0) < 0);

		test(5 == _syscall_read(ends[0], buf, 16, 0));
		test(!memcmp(buf, pipe_msgs[1], 5));

		_syscall_await();
	}
	close(ends[0]);
	close(ends[1]);


	/* writing to pipes with one end closed */
	test(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			close(ends[1]);
			test(_syscall_read(ends[0], buf, 16, 0) < 0);
			exit(0);
		}
	}
	close(ends[1]);
	for (int i = 0; i < 16; i++)
		_syscall_await();
	close(ends[0]);

	test(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			close(ends[0]);
			test(_syscall_write(ends[1], buf, 16, 0, 0) < 0);
			exit(0);
		}
	}
	close(ends[0]);
	for (int i = 0; i < 16; i++)
		_syscall_await();
	close(ends[1]);


	/* queueing */
	test(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			test(_syscall_write(ends[1], pipe_msgs[0], 5, -1, 0) == 5);
			exit(0);
		}
	}
	close(ends[1]);
	for (int i = 0; i < 16; i++) {
		test(_syscall_read(ends[0], buf, sizeof buf, 0) == 5);
		_syscall_await();
	}
	test(_syscall_read(ends[0], buf, sizeof buf, 0) < 0);
	close(ends[0]);

	test(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			memset(buf, 0, sizeof buf);
			test(_syscall_read(ends[0], buf, 5, -1) == 5);
			test(!memcmp(buf, pipe_msgs[1], 5));
			exit(0);
		}
	}
	close(ends[0]);
	for (int i = 0; i < 16; i++) {
		test(_syscall_write(ends[1], pipe_msgs[1], 5, -1, 0) == 5);
		_syscall_await();
	}
	test(_syscall_write(ends[1], pipe_msgs[1], 5, -1, 0) < 0);
	close(ends[1]);


	// not a to.do detect when all processes that can read are stuck on writing to the pipe and vice versa
	// it seems like linux just lets the process hang endlessly.
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
		test(_syscall_await() != 0); // test if the process crashed
	}

	char *pages[4];
	for (size_t i = 0; i < 4; i++) {
		pages[i] = _syscall_memflag(NULL, 4096, MEMFLAG_FINDFREE);
		test(pages[i] == _syscall_memflag(NULL, 4096, MEMFLAG_FINDFREE | MEMFLAG_PRESENT));
		if (i > 0) test(pages[i] != pages[i-1]);
		*pages[i] = 0x77;
	}

	for (size_t i = 0; i < 4; i++)
		_syscall_memflag(pages, 4096, MEMFLAG_PRESENT);
}

static void test_dup(void) {
	handle_t pipe[2];
	handle_t h1, h2;
	test(_syscall_pipe(pipe, 0) >= 0);

	if (!fork()) {
		close(pipe[0]);

		h1 = _syscall_dup(pipe[1], -1, 0);
		test(h1 >= 0);
		test(h1 != pipe[1]);
		h2 = _syscall_dup(h1, -1, 0);
		test(h2 >= 0);
		test(h2 != pipe[1] && h2 != h1);

		_syscall_write(pipe[1], "og", 2, 0, 0);
		_syscall_write(h1, "h1", 2, 0, 0);
		_syscall_write(h2, "h2", 2, 0, 0);

		close(pipe[1]);
		_syscall_write(h1, "h1", 2, 0, 0);
		_syscall_write(h2, "h2", 2, 0, 0);

		test(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		test(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		test(_syscall_dup(h1, pipe[1], 0) == pipe[1]);
		test(_syscall_dup(h2, pipe[1], 0) == pipe[1]);
		close(h1);
		close(h2);

		test(_syscall_dup(pipe[1], h2, 0) == h2);
		_syscall_write(h2, "h2", 2, 0, 0);
		close(h2);

		test(_syscall_dup(pipe[1], h1, 0) == h1);
		_syscall_write(h1, "h1", 2, 0, 0);
		close(h1);

		exit(0);
	} else {
		char buf[16];
		size_t count = 0;
		close(pipe[1]);
		while (_syscall_read(pipe[0], buf, sizeof buf, 0) >= 0)
			count++;
		test(count == 7);
		_syscall_await();
	}


	close(pipe[0]);
}

static void test_execbuf(void) {
	if (!fork()) {
		uint64_t buf[] = {
			EXECBUF_SYSCALL, _SYSCALL_EXIT, 123, 0, 0, 0, 0,
		};
		_syscall_execbuf(buf, sizeof buf);
		test_fail();
	} else {
		test(_syscall_await() == 123);
	}
}

static void test_sleep(void) {
	handle_t reader;
	FILE *writer;
	if (!forkpipe(&writer, &reader)) {
		if (!fork()) {
			if (!fork()) {
				_syscall_sleep(100);
				fprintf(writer, "1");
				_syscall_sleep(200);
				fprintf(writer, "3");
				_syscall_sleep(200);
				fprintf(writer, "5");
				_syscall_exit(0);
			}
			if (!fork()) {
				fprintf(writer, "0");
				_syscall_sleep(200);
				fprintf(writer, "2");
				_syscall_sleep(200);
				fprintf(writer, "4");
				/* get killed while asleep
				* a peaceful death, i suppose. */
				for (;;) _syscall_sleep(1000000000);
			}
			_syscall_await();
			_syscall_exit(0);
		}

		/* this part checks if, after killing an asleep process,
		* other processes can still wake up */
		_syscall_sleep(600);
		fprintf(writer, "6");
		exit(0);
	} else {
		const char *expected = "0123456";
		size_t target = strlen(expected);
		size_t pos = 0;
		for (;;) {
			char buf[128];
			long ret = _syscall_read(reader, buf, sizeof buf, 0);
			if (ret < 0) break;
			test(pos + ret <= target);
			test(memcmp(buf, expected + pos, ret) == 0);
			pos += ret;
		}
		test(pos == target);
	}
}

void r_k_miscsyscall(void) {
	run_test(test_await);
	run_test(test_pipe);
	run_test(test_memflag);
	run_test(test_dup);
	run_test(test_execbuf);
	run_test(test_sleep);
}

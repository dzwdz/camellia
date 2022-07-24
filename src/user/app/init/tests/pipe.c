#define TEST_MACROS
#include <user/lib/stdlib.h>
#include <user/app/init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static const char *pipe_msgs[2] = {"hello", "world"};

void test_pipe(void) {
	handle_t ends[2];
	char buf[16];
	int ret;

	/* test regular reads / writes */
	assert(_syscall_pipe(ends, 0) >= 0);
	if (!fork()) {
		/* those repeated asserts ensure that you can't read/write to the wrong ends */
		assert(_syscall_read(ends[1], buf, 16, 0) < 0);
		assert(_syscall_write(ends[0], buf, 16, 0) < 0);

		ret = _syscall_write(ends[1], pipe_msgs[0], 5, -1);
		assert(ret == 5);

		assert(_syscall_read(ends[1], buf, 16, 0) < 0);
		assert(_syscall_write(ends[0], buf, 16, 0) < 0);

		ret = _syscall_write(ends[1], pipe_msgs[1], 5, -1);
		assert(ret == 5);

		_syscall_exit(0);
	} else {
		assert(_syscall_read(ends[1], buf, 16, 0) < 0);
		assert(_syscall_write(ends[0], buf, 16, 0) < 0);

		ret = _syscall_read(ends[0], buf, 16, 0);
		assert(ret == 5);
		assert(!memcmp(buf, pipe_msgs[0], 5));

		assert(_syscall_read(ends[1], buf, 16, 0) < 0);
		assert(_syscall_write(ends[0], buf, 16, 0) < 0);

		_syscall_read(ends[0], buf, 16, 0);
		assert(ret == 5);
		assert(!memcmp(buf, pipe_msgs[1], 5));

		_syscall_await();
	}
	close(ends[0]);
	close(ends[1]);


	/* writing to pipes with one end closed */
	assert(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			close(ends[1]);
			assert(_syscall_read(ends[0], buf, 16, 0) < 0);
			_syscall_exit(0);
		}
	}
	close(ends[1]);
	for (int i = 0; i < 16; i++)
		_syscall_await();
	close(ends[0]);

	assert(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			close(ends[0]);
			assert(_syscall_write(ends[1], buf, 16, 0) < 0);
			_syscall_exit(0);
		}
	}
	close(ends[0]);
	for (int i = 0; i < 16; i++)
		_syscall_await();
	close(ends[1]);


	/* queueing */
	assert(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			assert(_syscall_write(ends[1], pipe_msgs[0], 5, -1) == 5);
			_syscall_exit(0);
		}
	}
	close(ends[1]);
	for (int i = 0; i < 16; i++) {
		assert(_syscall_read(ends[0], buf, sizeof buf, 0) == 5);
		_syscall_await();
	}
	assert(_syscall_read(ends[0], buf, sizeof buf, 0) < 0);
	close(ends[0]);

	assert(_syscall_pipe(ends, 0) >= 0);
	for (int i = 0; i < 16; i++) {
		if (!fork()) {
			memset(buf, 0, sizeof buf);
			assert(_syscall_read(ends[0], buf, 5, -1) == 5);
			assert(!memcmp(buf, pipe_msgs[1], 5));
			_syscall_exit(0);
		}
	}
	close(ends[0]);
	for (int i = 0; i < 16; i++) {
		assert(_syscall_write(ends[1], pipe_msgs[1], 5, -1) == 5);
		_syscall_await();
	}
	assert(_syscall_write(ends[1], pipe_msgs[1], 5, -1) < 0);
	close(ends[1]);


	// not a to.do detect when all processes that can read are stuck on writing to the pipe and vice versa
	// it seems like linux just lets the process hang endlessly.
}

#define TEST_MACROS
#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static const char *pipe_msgs[2] = {"hello", "world"};

static void test_pipe_child(handle_t ends[2]) {
	int ret = _syscall_write(ends[1], pipe_msgs[0], 5, -1);
	assert(ret == 5);

	ret = _syscall_write(ends[1], pipe_msgs[1], 5, -1);
	assert(ret == 5);
}

static void test_pipe_parent(handle_t ends[2]) {
	char buf[16];
	int ret = _syscall_read(ends[0], buf, 16, 0);
	assert(ret == 5);
	assert(!memcmp(buf, pipe_msgs[0], 5));

	_syscall_read(ends[0], buf, 16, 0);
	assert(ret == 5);
	assert(!memcmp(buf, pipe_msgs[1], 5));

	// TODO these calls fail for multiple reasons at once - split
	assert(_syscall_read(ends[1], buf, 16, 0) < 0);
	assert(_syscall_write(ends[0], buf, 16, 0) < 0);
}

void test_pipe(void) {
	handle_t ends[2];
	char buf[16];
	assert(_syscall_pipe(ends, 0) >= 0);

	if (!_syscall_fork(0, NULL)) {
		test_pipe_child(ends);
		_syscall_exit(0);
	} else {
		test_pipe_parent(ends);
		_syscall_await();
	}

	_syscall_close(ends[0]);
	_syscall_close(ends[1]);

	assert(_syscall_pipe(ends, 0) >= 0);
	_syscall_close(ends[0]);
	assert(_syscall_write(ends[1], buf, 16, 0) < 0);
	_syscall_close(ends[1]);

	assert(_syscall_pipe(ends, 0) >= 0);
	_syscall_close(ends[1]);
	assert(_syscall_read(ends[0], buf, 16, 0) < 0);
	_syscall_close(ends[0]);


	assert(_syscall_pipe(ends, 0) >= 0);
	if (!_syscall_fork(0, NULL)) {
		_syscall_exit(0);
	} else {
		_syscall_close(ends[1]);
		assert(_syscall_read(ends[0], buf, 16, 0) < 0);
		_syscall_await();
	}

	assert(_syscall_pipe(ends, 0) >= 0);
	if (!_syscall_fork(0, NULL)) {
		_syscall_exit(0);
	} else {
		_syscall_close(ends[1]);
		assert(_syscall_write(ends[1], buf, 16, 0) < 0);
		_syscall_await();
	}

	// TODO detect when all processes that can read are stuck on writing to the pipe and vice verse
	// TODO kill process that's waiting on a pipe
	// TODO queue
}

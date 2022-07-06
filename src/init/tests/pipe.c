#define TEST_MACROS
#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>

static const char *pipe_msgs[2] = {"hello", "world"};

static void test_pipe_child(handle_t pipe) {
	int ret = _syscall_write(pipe, pipe_msgs[0], 5, -1);
	assert(ret == 5);

	ret = _syscall_write(pipe, pipe_msgs[1], 5, -1);
	assert(ret == 5);
}

static void test_pipe_parent(handle_t pipe) {
	char buf[16];
	int ret = _syscall_read(pipe, buf, 16, 0);
	assert(ret == 5);
	assert(!memcmp(buf, pipe_msgs[0], 5));

	_syscall_read(pipe, buf, 16, 0);
	assert(ret == 5);
	assert(!memcmp(buf, pipe_msgs[1], 5)); // wrong compare for test
}

void test_pipe(void) {
	handle_t pipe = _syscall_pipe(0);
	assert(pipe > 0);

	if (!_syscall_fork(0, NULL)) {
		test_pipe_child(pipe);
		_syscall_exit(0);
	} else {
		test_pipe_parent(pipe);
		_syscall_await();
	}

	// TODO kill process that's waiting on a pipe
}

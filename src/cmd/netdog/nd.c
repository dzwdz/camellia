#include <camellia.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

#define eprintf(fmt, ...) fprintf(stderr, "netdog: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

hid_t conn;

void send_stdin(void *arg) { (void)arg;
	static char buf[4096];
	for (;;) {
		// TODO define STDIN_FILENO
		long ret = _sys_read(0, buf, sizeof buf, -1);
		if (ret <= 0) return; /* instead of sending an empty packet, quit. */
		ret = _sys_write(conn, buf, ret, -1, 0);
		if (ret < 0) return;
	}
}

void recv_stdout(void *arg) { (void)arg;
	static char buf[4096];
	for (;;) {
		long ret = _sys_read(conn, buf, sizeof buf, -1);
		if (ret < 0) return;
		ret = _sys_write(1, buf, ret, -1, 0);
		if (ret < 0) return;
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		eprintf("no argument");
		return 1;
	}

	conn = camellia_open(argv[1], OPEN_RW);
	if (conn < 0) {
		eprintf("couldn't open '%s', err %u", argv[1], -conn);
		return -conn;
	}

	thread_create(0, send_stdin, NULL);
	thread_create(0, recv_stdout, NULL);
	_sys_await();
	return 0;
}

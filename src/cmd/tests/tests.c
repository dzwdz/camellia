#include "tests.h"
#include <camellia/syscalls.h>
#include <sys/wait.h>
#include <unistd.h>

FILE *fail_trig;

void run_test_inner(void (*fn)(), const char *s) {
	int pid = fork();
	if (pid == 0) {
		fn();
		_sys_filicide();
		exit(0);
	} else if (pid < 0) {
		/* working around macro stupidity */
		test_failf("%s", "in fork");
	} else {
		/* successful tests must return 0 */
		int status;
		if (waitpid(pid, &status, 0) != pid) {
			test_failf("%s", "waitpid returned something weird");
		} else if (WEXITSTATUS(status) != 0) {
			test_failf("%s exited with %d", s, WEXITSTATUS(status));
		}
	}
}

int forkpipe(FILE **f, hid_t *h) {
	hid_t ends[2];
	if (_sys_pipe(ends, 0) < 0) {
		fprintf(stderr, "couldn't create pipe\n");
		exit(1);
	}
	int ret = fork();
	if (!ret) {
		close(ends[0]);
		*f = fdopen(ends[1], "w");
		*h = -1;
	} else {
		close(ends[1]);
		*f = NULL;
		*h = ends[0];
	}
	return ret;
}

int main(void) {
	hid_t reader;
	if (!forkpipe(&fail_trig, &reader)) {
		r_k_miscsyscall();
		r_k_fs();
		r_k_fdlimit();
		r_k_misc();
		r_k_path();
		r_k_threads();
		r_libc_esemaphore();
		r_libc_setjmp();
		r_libc_string();
		r_s_printf();
		r_s_ringbuf();
		exit(0);
	} else {
		for (;;) {
			char buf[128];
			long ret = _sys_read(reader, buf, sizeof buf, 0);
			if (ret < 0) break;
			printf("\033[31mFAIL\033[0m ");
			fwrite(buf, ret, 1, stdout);
			printf("\n");
		}
	}
	return 0;
}

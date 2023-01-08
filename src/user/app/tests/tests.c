#include "tests.h"
#include <camellia/syscalls.h>
#include <unistd.h>

__attribute__((visibility("hidden")))
extern char _image_base[];

FILE *fail_trig;

void run_test(void (*fn)()) {
	if (!fork()) {
		fn();
		_syscall_filicide();
		exit(0);
	} else {
		/* successful tests must return 0 */
		if (_syscall_await() != 0) {
			test_failf("%p, base %p", (void*)fn - (void*)_image_base, _image_base);
		}
	}
}

int forkpipe(FILE **f, handle_t *h) {
	handle_t ends[2];
	if (_syscall_pipe(ends, 0) < 0) {
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
	handle_t reader;
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
			long ret = _syscall_read(reader, buf, sizeof buf, 0);
			if (ret < 0) break;
			printf("\033[31mFAIL\033[0m ");
			fwrite(buf, ret, 1, stdout);
			printf("\n");
		}
	}
	return 0;
}

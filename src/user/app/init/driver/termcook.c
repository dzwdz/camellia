#include "driver.h"
#include <camellia/syscalls.h>
#include <stdio.h>
#include <unistd.h>

static void w_output(handle_t output, const char *buf, size_t len) {
	size_t pos = 0;
	while (pos < len) {
		int ret = _syscall_write(output, buf + pos, len - pos, pos);
		if (ret < 0) break;
		pos += ret;
	}
}

static void line_editor(handle_t input, handle_t output) {
	char readbuf[16], linebuf[256];
	size_t linepos = 0;
	for (;;) {
		int readlen = _syscall_read(input, readbuf, sizeof readbuf, -1);
		if (readlen < 0) return;
		for (int i = 0; i < readlen; i++) {
			char c = readbuf[i];
			switch (c) {
				case '\b':
				case 0x7f:
					if (linepos != 0) {
						printf("\b \b");
						linepos--;
					}
					break;
				case 4: /* EOT, C-d */
					if (linepos > 0) {
						w_output(output, linebuf, linepos);
						linepos = 0;
					} else {
						_syscall_write(output, NULL, 0, 0); // eof
					}
					break;
				case '\n':
				case '\r':
					printf("\n");
					if (linepos < sizeof linebuf)
						linebuf[linepos++] = '\n';
					w_output(output, linebuf, linepos);
					linepos = 0;
					break;
				default:
					if (linepos < sizeof linebuf) {
						linebuf[linepos++] = c;
						printf("%c", c);
					}
					break;
			}
		}
	}
}

void termcook(void) {
	handle_t stdin_pipe[2] = {-1, -1};
	if (_syscall_pipe(stdin_pipe, 0) < 0)
		return;

	if (!fork()) {
		close(stdin_pipe[0]);
		line_editor(0, stdin_pipe[1]);
		exit(0);
	}
	/* 0 is stdin, like in unix */
	_syscall_dup(stdin_pipe[0], 0, 0);
	close(stdin_pipe[0]);
	close(stdin_pipe[1]);
}

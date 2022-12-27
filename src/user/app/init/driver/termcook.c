#include "driver.h"
#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum tstate {
	Normal,
	Esc,
	CSI,
};

static void w_output(handle_t output, const char *buf, size_t len) {
	size_t pos = 0;
	while (pos < len) {
		int ret = _syscall_write(output, buf + pos, len - pos, pos, 0);
		if (ret < 0) break;
		pos += ret;
	}
}

static void line_editor(handle_t input, handle_t output) {
	char readbuf[16], linebuf[256];
	size_t linepos = 0;
	enum tstate state = Normal;
	for (;;) {
		int readlen = _syscall_read(input, readbuf, sizeof readbuf, -1);
		if (readlen < 0) return;
		for (int i = 0; i < readlen; i++) {
			char c = readbuf[i];
			switch (state) {
			case Normal:
				switch (c) {
				case '\b':
				case 0x7f:
					if (linepos != 0) {
						printf("\b \b");
						linepos--;
					}
					break;
				case 3: /* C-c */
					_syscall_exit(1);
				case 4: /* EOT, C-d */
					if (linepos > 0) {
						w_output(output, linebuf, linepos);
						linepos = 0;
					} else {
						_syscall_write(output, NULL, 0, 0, 0); /* EOF */
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
				case '\e':
					state = Esc;
					break;
				case '\t':
					break;
				default:
					if (linepos < sizeof linebuf) {
						linebuf[linepos++] = c;
						printf("%c", c);
					}
					break;
				}
				break;
			case Esc:
				if (c == '[') state = CSI;
				else state = Normal;
				break;
			case CSI:
				if (0x40 <= c && c <= 0x7E) state = Normal;
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
		/* the caller continues in a child process,
		 * so it can be killed when the line editor quits */
		_syscall_dup(stdin_pipe[0], 0, 0);
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		return;
	}
	if (!fork()) {
		close(stdin_pipe[0]);
		line_editor(0, stdin_pipe[1]);
		exit(0);
	}
	exit(_syscall_await());
}

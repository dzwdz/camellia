#include <init/shell.h>
#include <init/stdlib.h>
#include <shared/syscalls.h>

#define PROMPT "$ "

static int tty_fd = 0; // TODO put in stdlib

static char *split(char *base) {
	while (*base) {
		if (*base == ' ' || *base == '\t') {
			*base++ = '\0';
			return base;
		}
		base++;
	}
	return NULL;
}

static int readline(char *buf, size_t max) {
	char c;
	size_t pos = 0;
	while (_syscall_read(tty_fd, &c, 1, 0)) {
		switch (c) {
			case '\b':
			case 0x7f:
				/* for some reason backspace outputs 0x7f (DEL) */
				if (pos != 0) {
					printf("\b \b");
					pos--;
				}
				break;
			case '\r':
				printf("\n");
				buf[pos++] = '\0';
				return pos;
			default:
				if (pos < max) {
					_syscall_write(tty_fd, &c, 1, 0);
					buf[pos] = c;
					pos++;
				}
		}
	}
	return -1; // error
}

static void cmd_cat(const char *args) {
	int fd = _syscall_open(args, strlen(args));
	static char buf[256];
	int len = 256;

	if (fd < 0) {
		printf("couldn't open.\n");
		return;
	}

	len = _syscall_read(fd, buf, len, 0);
	_syscall_write(tty_fd, buf, len, 0);
	_syscall_close(fd);
}

void shell_loop(void) {
	static char cmd[256];
	char *args;

	for (;;) {
		printf(PROMPT);
		readline(cmd, 256);
		args = split(cmd);
		if (!strcmp(cmd, "echo")) {
			printf("%s\n", args);
		} else if (!strcmp(cmd, "cat")) {
			cmd_cat(args);
		} else {
			printf("unknown command :(\n");
		}
	}
}
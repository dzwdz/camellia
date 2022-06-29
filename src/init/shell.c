#include <init/shell.h>
#include <init/stdlib.h>
#include <init/tests/main.h>
#include <shared/syscalls.h>
#include <stdbool.h>

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
	while (file_read(&__stdin, &c, 1)) {
		switch (c) {
			case '\b':
			case 0x7f:
				/* for some reason backspace outputs 0x7f (DEL) */
				if (pos != 0) {
					printf("\b \b");
					pos--;
				}
				break;
			case '\n':
			case '\r':
				printf("\n");
				buf[pos++] = '\0';
				return pos;
			default:
				if (pos < max) {
					printf("%c", c);
					buf[pos] = c;
					pos++;
				}
		}
	}
	return -1; // error
}

static void cmd_cat_ls(const char *args, bool ls) {
	libc_file file;
	static char buf[512];
	int len; // first used for strlen(args), then length of buffer

	if (!args) args = "/";
	len = strlen(args);
	memcpy(buf, args, len + 1); // no overflow check - the shell is just a PoC

	if (ls) { // paths to directories always have a trailing slash
		char *p = buf;
		while (*p) p++;
		if (p[-1] != '/') {
			p[0] = '/';
			p[1] = '\0';
			len++;
		}
	}

	if (file_open(&file, buf, 0) < 0) {
		printf("couldn't open.\n");
		return;
	}

	while (!file.eof) {
		int len = file_read(&file, buf, sizeof buf);
		if (len <= 0) break;

		if (ls) {
			for (int i = 0; i < len; i++)
				if (buf[i] == '\0') buf[i] = '\n';
		}
		file_write(&__stdout, buf, len);
	}
	file_close(&file);
}

static void cmd_hexdump(const char *args) {
	static uint8_t buf[512];
	int fd, len;

	fd = _syscall_open(args, strlen(args), 0);
	if (fd < 0) {
		printf("couldn't open.\n");
		return;
	}

	len = _syscall_read(fd, buf, sizeof buf, 0);
	for (int i = 0; i < len; i += 16) {
		printf("%08x  ", i);

		for (int j = i; j < i + 8 && j < len; j++)
			printf("%02x ", buf[j]);
		printf(" ");
		for (int j = i + 8; j < i + 16 && j < len; j++)
			printf("%02x ", buf[j]);
		printf(" |");

		for (int j = i; j < i + 16 && j < len; j++) {
			char c = '.';
			if (0x20 <= buf[j] && buf[j] < 0x7f) c = buf[j];
			printf("%c", c);
		}
		printf("|\n");
	}

	_syscall_close(fd);
}

static void cmd_touch(const char *args) {
	int fd = _syscall_open(args, strlen(args), OPEN_CREATE);
	if (fd < 0) {
		printf("couldn't create file.\n");
		return;
	}
	_syscall_close(fd);
}

void shell_loop(void) {
	static char cmd[256];
	int level = 0;
	char *args;

	for (;;) {
		printf("%x$ ", level);
		readline(cmd, 256);
		args = split(cmd);
		if (!strcmp(cmd, "echo")) {
			printf("%s\n", args);
		} else if (!strcmp(cmd, "cat")) {
			cmd_cat_ls(args, false);
		} else if (!strcmp(cmd, "ls")) {
			cmd_cat_ls(args, true);
		} else if (!strcmp(cmd, "hexdump")) {
			cmd_hexdump(args);
		} else if (!strcmp(cmd, "catall")) {
			const char *files[] = {
				"/init/fake.txt",
				"/init/1.txt", "/init/2.txt",
				"/init/dir/3.txt", NULL};
			for (int i = 0; files[i]; i++) {
				printf("%s:\n", files[i]);
				cmd_cat_ls(files[i], false);
				printf("\n");
			}
		} else if (!strcmp(cmd, "touch")) {
			cmd_touch(args);
		} else if (!strcmp(cmd, "shadow")) {
			_syscall_mount(-1, args, strlen(args));
		} else if (!strcmp(cmd, "exit")) {
			_syscall_exit(0);
		} else if (!strcmp(cmd, "fork")) {
			if (_syscall_fork(0, NULL))
				_syscall_await();
			else level++;
		} else if (!strcmp(cmd, "run_tests")) {
			test_all();
		} else {
			printf("unknown command :(\n");
		}
	}
}

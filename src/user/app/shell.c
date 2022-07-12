#include <user/app/shell.h>
#include <user/lib/stdlib.h>
#include <user/tests/main.h>
#include <shared/syscalls.h>
#include <stdbool.h>

static bool isspace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

static char *strsplit(char *base, char delim) {
	if (!base) return NULL;
	while (*base) {
		if (delim ? *base == delim : isspace(*base)) {
			*base++ = '\0';
			return base;
		}
		base++;
	}
	return NULL;
}

static char *strtrim(char *s) {
	char *end;
	if (!s) return NULL;
	while (isspace(*s)) s++;
	end = s + strlen(s);
	while (end > s && isspace(end[-1])) end--;
	*end = '\0';
	return s;
}


static int readline(char *buf, size_t max) {
	char c;
	size_t pos = 0;
	while (file_read(stdin, &c, 1)) {
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
	libc_file *file;
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

	file = file_open(buf, 0);
	if (!file) {
		printf("couldn't open.\n");
		return;
	}

	while (!file->eof) {
		int len = file_read(file, buf, sizeof buf);
		if (len <= 0) break;

		if (ls) {
			for (int i = 0; i < len; i++)
				if (buf[i] == '\0') buf[i] = '\n';
		}
		file_write(stdout, buf, len);
	}
	file_close(file);
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

	close(fd);
}

static void cmd_touch(const char *args) {
	int fd = _syscall_open(args, strlen(args), OPEN_CREATE);
	if (fd < 0) {
		printf("couldn't create file.\n");
		return;
	}
	close(fd);
}

void shell_loop(void) {
	static char buf[256];
	int level = 0;
	char *cmd, *args, *redir;

	for (;;) {
		printf("%x$ ", level);

		readline(buf, 256);
		redir = strtrim(strsplit(buf, '>'));
		cmd = strtrim(buf);
		args = strtrim(strsplit(cmd, 0));

		/* "special" commands that can't be handled in a subprocess */
		if (!strcmp(cmd, "shadow")) {
			_syscall_mount(-1, args, strlen(args));
			continue;
		} else if (!strcmp(cmd, "exit")) {
			_syscall_exit(0);
			continue;
		} else if (!strcmp(cmd, "fork")) {
			if (!fork()) level++;
			else _syscall_await();
			continue;
		}

		if (!fork()) {
			if (redir && !file_reopen(stdout, redir, OPEN_CREATE)) {
				// TODO stderr
				_syscall_exit(0);
			}

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
			} else if (!strcmp(cmd, "run_tests")) {
				test_all();
			} else if (!strcmp(cmd, "stress")) {
				stress_all();
			} else {
				printf("unknown command :(\n");
			}
			_syscall_exit(0);
		} else {
			_syscall_await();
		}
	}
}

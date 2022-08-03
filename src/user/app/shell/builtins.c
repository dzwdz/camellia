#include "builtins.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void cmd_cat(int argc, const char **argv) {
	// TODO loop over argv

	FILE *file;
	static char buf[512];

	if (argv[1])
		file = fopen(argv[1], "r");
	else
		file = file_clone(stdin, "r");

	if (!file) {
		eprintf("couldn't open");
		return;
	}

	for (;;) {
		int len = fread(buf, 1, sizeof buf, file);
		if (len <= 0) break;
		fwrite(buf, 1, len, stdout);
	}
	fclose(file);
}

static void cmd_echo(int argc, const char **argv) {
	bool newline = true;
	int i = 1;

	if (!strcmp("-n", argv[i])) {
		i++;
		newline = false;
	}

	printf("%s", argv[i++]);
	for (; argv[i]; i++)
		printf(" %s", argv[i]);
	if (newline)
		printf("\n");
}

void cmd_hexdump(int argc, const char **argv) {
	// TODO loop over argv
	// TODO use fopen/fread
	static uint8_t buf[512];
	int fd, len;

	fd = _syscall_open(argv[1], strlen(argv[1]), 0);
	if (fd < 0) {
		eprintf("couldn't open %s", argv[1]);
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

static void cmd_ls(int argc, const char **argv) {
	// TODO loop over argv
	FILE *file;
	static char buf[512];

	if (argv[1]) {
		int len = strlen(argv[1]);
		memcpy(buf, argv[1], len + 1); // TODO no overflow check

		if (buf[len-1] != '/') {
			buf[len] = '/';
			buf[len+1] = '\0';
		}

		file = fopen(buf, "r");
	} else {
		file = fopen("/", "r");
	}

	if (!file) {
		eprintf("couldn't open");
		return;
	}

	for (;;) {
		int len = fread(buf, 1, sizeof buf, file);
		if (len <= 0) break;
		for (int i = 0; i < len; i++)
			if (buf[i] == '\0') buf[i] = '\n';
		fwrite(buf, 1, len, stdout);
	}
	fclose(file);
}

static void cmd_touch(int argc, const char **argv) {
	// TODO loop over argv
	int fd = _syscall_open(argv[1], strlen(argv[1]), OPEN_CREATE);
	if (fd < 0) {
		eprintf("couldn't touch %s\n", argv[1]);
		return;
	}
	close(fd);
}

struct builtin builtins[] = {
	{"cat", cmd_cat},
	{"echo", cmd_echo},
	{"hexdump", cmd_hexdump},
	{"ls", cmd_ls},
	{"touch", cmd_touch},
	{NULL, NULL},
};

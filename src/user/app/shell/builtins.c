#include "builtins.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void cmd_cat_ls(const char *args, bool ls) {
	FILE *file;
	static char buf[512];
	int len; // first used for strlen(args), then length of buffer

	if (args) {
		len = strlen(args);
		memcpy(buf, args, len + 1); // no overflow check - the shell is just a PoC

		if (ls) { // paths to directories always have a trailing slash
			if (buf[len-1] != '/') {
				buf[len] = '/';
				buf[len+1] = '\0';
			}
		}

		file = fopen(buf, "r");
	} else if (ls) { /* ls default argument */
		file = fopen("/", "r");
	} else { /* cat default argument */
		file = file_clone(stdin);
	}

	if (!file) {
		eprintf("couldn't open");
		return;
	}

	while (!feof(file)) {
		int len = fread(buf, 1, sizeof buf, file);
		if (len <= 0) break;

		if (ls) {
			for (int i = 0; i < len; i++)
				if (buf[i] == '\0') buf[i] = '\n';
		}
		fwrite(buf, 1, len, stdout);
	}
	fclose(file);
}

void cmd_hexdump(const char *args) {
	static uint8_t buf[512];
	int fd, len;

	fd = _syscall_open(args, strlen(args), 0);
	if (fd < 0) {
		eprintf("couldn't open %s", args);
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

void cmd_touch(const char *args) {
	int fd = _syscall_open(args, strlen(args), OPEN_CREATE);
	if (fd < 0) {
		eprintf("couldn't touch %s\n", args);
		return;
	}
	close(fd);
}

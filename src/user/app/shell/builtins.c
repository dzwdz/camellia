#include "builtins.h"
#include "shell.h"
#include <camellia.h>
#include <camellia/fs/misc.h>
#include <camellia/path.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_ARGV(...) \
	char *_argv_default[] = {argv[0], __VA_ARGS__}; \
	if (argc <= 1) { \
		argc = sizeof(_argv_default) / sizeof(*_argv_default); \
		argv = _argv_default; \
	}

static void cmd_cat(int argc, char **argv) {
	const size_t buflen = 4096;
	char *buf = malloc(buflen);

	DEFAULT_ARGV("!stdin");
	for (int i = 1; i < argc; i++) {
		FILE *file = fopen(argv[i], "r");
		if (!file) {
			perror(argv[i]);
			return;
		}
		if (!strcmp(argv[i], "!stdin")) fextflags(file, FEXT_NOFILL);
		for (;;) {
			int len = fread(buf, 1, buflen, file);
			if (len <= 0) break;
			fwrite(buf, 1, len, stdout);
		}
		fclose(file);
	}
}

static void cmd_echo(int argc, char **argv) {
	bool newline = true;
	int i = 1;

	if (!strcmp("-n", argv[i])) {
		i++;
		newline = false;
	}

	printf("%s", argv[i++]);
	for (; i < argc; i++)
		printf(" %s", argv[i]);
	if (newline)
		printf("\n");
}

void cmd_getsize(int argc, char **argv) {
	if (argc < 2) errx(1, "no arguments");
	for (int i = 1; i < argc; i++) {
		handle_t h = camellia_open(argv[i], OPEN_READ);
		if (h < 0) {
			warn("error opening %s", argv[i]);
			continue;
		}
		printf("%s: %d\n", argv[i], (int)_syscall_getsize(h));
		_syscall_close(h);
	}
}

void cmd_hexdump(int argc, char **argv) {
	DEFAULT_ARGV("!stdin");
	const size_t buflen = 4096;
	uint8_t *buf = malloc(buflen);
	FILE *file;
	bool canonical = strcmp(argv[0], "hd") == 0;
	size_t readlen = ~0;
	size_t pos = 0;
	bool raw = false;

	int c;
	optind = 0;
	while ((c = getopt(argc, argv, "Cn:s:r")) != -1) {
		switch (c) {
			case 'C':
				canonical = true;
				break;
			case 'n':
				readlen = strtol(optarg, NULL, 0);
				break;
			case 's':
				pos = strtol(optarg, NULL, 0);
				break;
			case 'r': /* "raw" mode, print data as soon as it's read without buffering */
				raw = true;
				break;
			default:
				return;
		}
	}
	if (readlen != (size_t)~0)
		readlen += pos;

	for (; optind < argc; optind++) {
		file = fopen(argv[optind], "r");
		if (!file) {
			warn("couldn't open %s", argv[optind]);
			continue;
		}
		if (raw) fextflags(file, FEXT_NOFILL);
		fseek(file, pos, SEEK_SET);
		bool skipped = false;
		while (pos < readlen) {
			size_t len = buflen;
			if (len > readlen - pos)
				len = readlen - pos;
			len = fread(buf, 1, len, file);
			if (len == 0) break;

			for (size_t i = 0; i < len; i += 16) {
				if (i >= 16 && !memcmp(buf + i, buf + i - 16, 16)) {
					if (!skipped) {
						printf("*\n");
						skipped = true;
					}
					continue;
				} else skipped = false;
				printf("%08x  ", pos + i);

				for (size_t j = i; j < i + 8 && j < len; j++)
					printf("%02x ", buf[j]);
				printf(" ");
				for (size_t j = i + 8; j < i + 16 && j < len; j++)
					printf("%02x ", buf[j]);

				if (canonical) {
					printf(" |");

					for (size_t j = i; j < i + 16 && j < len; j++) {
						char c = '.';
						if (0x20 <= buf[j] && buf[j] < 0x7f) c = buf[j];
						printf("%c", c);
					}
					printf("|\n");
				} else {
					printf("\n");
				}
			}
			pos += len;
		}
		printf("%08x\n", pos);
		fclose(file);
	}
}

static void cmd_ls(int argc, char **argv) {
	FILE *file;
	const size_t buflen = 4096;
	char *buf = malloc(buflen);

	DEFAULT_ARGV(".");
	for (int i = 1; i < argc; i++) {
		char *path = (void*)argv[i];
		int pathlen = strlen(path);

		if (!pathlen || path[pathlen - 1] != '/') {
			memcpy(buf, path, pathlen);
			buf[pathlen] = '/';
			buf[pathlen+1] = '\0';
			path = buf;
		}

		file = fopen(path, "r");
		if (!file) {
			warn("couldn't open %s", argv[i]);
			continue;
		}
		for (;;) {
			int len = fread(buf, 1, buflen, file);
			if (len <= 0) break;
			for (int i = 0; i < len; i++)
				if (buf[i] == '\0') buf[i] = '\n';
			fwrite(buf, 1, len, stdout);
		}
		fclose(file);
	}
}

static void cmd_mkdir(int argc, char **argv) {
	// TODO mkdir -p
	if (argc < 2) errx(1, "no arguments");
	for (int i = 1; i < argc; i++) {
		if (mkdir(argv[i], 0777) < 0)
			perror(argv[i]);
	}
}

static void cmd_rm(int argc, char **argv) {
	if (argc < 2) errx(1, "no arguments");
	for (int i = 1; i < argc; i++) {
		if (unlink(argv[i]) < 0)
			perror(argv[i]);
	}
}

static void cmd_sleep(int argc, char **argv) {
	if (argc < 2) errx(1, "no arguments");
	_syscall_sleep(strtol(argv[1], NULL, 0) * 1000);
}

static void cmd_touch(int argc, char **argv) {
	if (argc < 2) errx(1, "no arguments");
	for (int i = 1; i < argc; i++) {
		FILE *f = fopen(argv[i], "a");
		if (!f) perror(argv[i]);
		fclose(f);
	}
}

static void cmd_whitelist(int argc, char **argv) {
	int split = 1;
	for (; split < argc;) {
		if (!strcmp("--", argv[split])) break;
		split++;
	}
	argv[split] = NULL;
	MOUNT_AT("/") { fs_whitelist((void*)&argv[1]); }

	if (split < argc) {
		run_args(argc - split - 1, &argv[split + 1], NULL);
	} else {
		const char **argv = (const char*[]){"shell", NULL};
		run_args(1, (void*)argv, NULL);
	}
}

struct builtin builtins[] = {
	{"cat", cmd_cat},
	{"echo", cmd_echo},
	{"getsize", cmd_getsize},
	{"hd", cmd_hexdump},
	{"hexdump", cmd_hexdump},
	{"ls", cmd_ls},
	{"mkdir", cmd_mkdir},
	{"rm", cmd_rm},
	{"sleep", cmd_sleep},
	{"touch", cmd_touch},
	{"whitelist", cmd_whitelist},
	{NULL, NULL},
};

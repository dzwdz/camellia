#include "builtins.h"
#include "shell.h"
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main();

static void execp(char **argv) {
	if (!argv || !*argv) return;
	if (argv[0][0] == '/') {
		execv(argv[0], argv);
		return;
	}

	size_t cmdlen = strlen(argv[0]);
	char *s = malloc(cmdlen);
	if (!s) {
		printf("sh: out of memory.\n");
		exit(1);
	}
	memcpy(s, "/bin/", 5);
	memcpy(s + 5, argv[0], cmdlen + 1);
	argv[0] = s;

	execv(s, argv);
	free(s);
}

static void run(char *cmd) {
	#define ARGV_MAX 16
	char *argv[ARGV_MAX];
	char *redir;

	int ret = parse(cmd, argv, ARGV_MAX, &redir);
	if (ret < 0) {
		printf("sh: error parsing command\n");
		return;
	}

	if (!*argv) return;

	/* "special" commands that can't be handled in a subprocess */
	if (!strcmp(argv[0], "shadow")) {
		// TODO process groups
		_syscall_mount(-1, argv[1], strlen(argv[1]));
		return;
	} else if (!strcmp(argv[0], "exit")) {
		exit(0);
	}

	if (fork()) {
		_syscall_await();
		return;
	}

	if (redir && !freopen(redir, "w", stdout)) {
		// TODO stderr
		exit(0);
	}

	if (!strcmp(argv[0], "echo")) {
		printf("%s", argv[1]);
		for (int i = 2; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
	} else if (!strcmp(argv[0], "fork")) {
		main();
	} else if (!strcmp(argv[0], "cat")) {
		// TODO better argv handling
		cmd_cat_ls(argv[1], false);
	} else if (!strcmp(argv[0], "ls")) {
		cmd_cat_ls(argv[1], true);
	} else if (!strcmp(argv[0], "hexdump")) {
		cmd_hexdump(argv[1]);
	} else if (!strcmp(argv[0], "catall")) {
		const char *files[] = {
			"/init/fake.txt",
			"/init/1.txt", "/init/2.txt",
			"/init/dir/3.txt", NULL};
		for (int i = 0; files[i]; i++) {
			printf("%s:\n", files[i]);
			cmd_cat_ls(files[i], false);
			printf("\n");
		}
	} else if (!strcmp(argv[0], "touch")) {
		cmd_touch(argv[1]);
	} else {
		execp(argv);
		if (errno == EINVAL) {
			printf("%s isn't a valid executable\n", argv[0]);
		} else {
			printf("unknown command: %s\n", argv[0]);
		}
	}
	exit(0); /* kills the subprocess */
}


int main(int argc, char **argv) {
	static char buf[256];
	FILE *f = stdin;

	if (argc > 1) {
		f = fopen(argv[1], "r");
		if (!f) {
			printf("sh: couldn't open %s\n", argv[1]);
			return 1;
		}
	}

	for (;;) {
		if (f == stdin)
			printf("$ ");
		if (!fgets(buf, 256, f))
			return 0;
		run(buf);
	}
}

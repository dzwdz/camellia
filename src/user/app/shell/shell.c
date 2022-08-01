#include "builtins.h"
#include "shell.h"
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/fs/misc.h>
#include <x86intrin.h>

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
		eprintf("out of memory.");
		exit(1);
	}
	memcpy(s, "/bin/", 5);
	memcpy(s + 5, argv[0], cmdlen + 1);
	argv[0] = s;

	execv(s, argv);
	free(s);
}

static void run_args(int argc, char **argv, struct redir *redir) {
	if (!*argv) return;

	/* "special" commands that can't be handled in a subprocess */
	if (!strcmp(argv[0], "shadow")) {
		// TODO process groups
		_syscall_mount(-1, argv[1], strlen(argv[1]));
		return;
	} else if (!strcmp(argv[0], "whitelist")) {
		MOUNT_AT("/") {
			fs_whitelist((void*)&argv[1]);
		}
		return;
	} else if (!strcmp(argv[0], "time")) {
		uint64_t time = __rdtsc();
		uint64_t div = 3000;
		run_args(argc - 1, argv + 1, redir);
		time = __rdtsc() - time;
		printf("0x%x ns (assuming 3GHz)\n", time / div);
		return;
	} else if (!strcmp(argv[0], "exit")) {
		exit(0);
	}

	if (fork()) {
		_syscall_await();
		return;
	}

	if (redir->stdout && !freopen(redir->stdout, redir->append ? "a" : "w", stdout)) {
		eprintf("couldn't open %s for redirection", redir->stdout);
		exit(0);
	}

	for (struct builtin *iter = builtins; iter->name; iter++) {
		if (!strcmp(argv[0], iter->name)) {
			iter->fn(argc, argv);
			exit(0);
		}
	}

	execp(argv);
	if (errno == EINVAL) {
		eprintf("%s isn't a valid executable\n", argv[0]);
	} else {
		eprintf("unknown command: %s\n", argv[0]);
	}
	exit(0); /* kills the subprocess */
}

static void run(char *cmd) {
	#define ARGV_MAX 16
	char *argv[ARGV_MAX];
	struct redir redir;

	int argc = parse(cmd, argv, ARGV_MAX, &redir);
	if (argc < 0) {
		eprintf("error parsing command");
		return;
	}

	run_args(argc, argv, &redir);
}


int main(int argc, char **argv) {
	static char buf[256];
	FILE *f = stdin;

	if (argc > 1) {
		f = fopen(argv[1], "r");
		if (!f) {
			eprintf("couldn't open %s\n", argv[1]);
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

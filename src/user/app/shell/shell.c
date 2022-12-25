#include "builtins.h"
#include "shell.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <camellia/fs/misc.h>
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

void run_args(int argc, char **argv, struct redir *redir) {
	if (!*argv) return;

	/* "special" commands that can't be handled in a subprocess */
	if (!strcmp(argv[0], "mount")) {
		if (argc < 3) {
			eprintf("not enough arguments");
			return;
		}
		MOUNT_AT(argv[1]) {
			run_args(argc - 2, argv + 2, redir);
			exit(1);
		}
		return;
	} else if (!strcmp(argv[0], "cd")) {
		if (chdir(argc > 1 ? argv[1] : "/") < 0)
			perror("cd");
		return;
	} else if (!strcmp(argv[0], "time")) {
		uint64_t time = __rdtsc();
		uint64_t div = 3000;
		run_args(argc - 1, argv + 1, redir);
		time = __rdtsc() - time;
		printf("%u ns (assuming 3GHz)\n", time / div);
		return;
	} else if (!strcmp(argv[0], "exit")) {
		exit(0);
	}

	if (fork()) {
		_syscall_await();
		return;
	}

	if (redir && redir->stdout) {
		FILE *f = fopen(redir->stdout, redir->append ? "a" : "w");
		if (!f) {
			eprintf("couldn't open %s for redirection", redir->stdout);
			exit(1);
		}

		/* a workaround for file offsets not being preserved across exec()s.
		 * TODO document that weird behaviour of exec() */

		handle_t p[2];
		if (_syscall_pipe(p, 0) < 0) {
			eprintf("couldn't create redirection pipe", redir->stdout);
			exit(1);
		}
		if (!_syscall_fork(FORK_NOREAP, NULL)) {
			/* the child forwards data from the pipe to the file */
			const size_t buflen = 512;
			char *buf = malloc(buflen);
			if (!buf) {
				eprintf("when redirecting: malloc failure");
				exit(1);
			}
			close(p[1]);
			for (;;) {
				long len = _syscall_read(p[0], buf, buflen, 0);
				if (len < 0) exit(0);
				fwrite(buf, 1, len, f);
				if (ferror(f)) exit(0);
			}
		}

		fclose(f);
		close(p[0]);
		if (_syscall_dup(p[1], 1, 0) < 0) {
			eprintf("dup failure");
			exit(1);
		}
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
			printf("%s $ ", getcwd(buf, sizeof buf));
		if (!fgets(buf, 256, f))
			return 0;
		run(buf);
	}
}

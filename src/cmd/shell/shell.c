#include "builtins.h"
#include "shell.h"
#include <camellia/compat.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <err.h>
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
	if (strncmp(argv[0], "/", 1) == 0 ||
		strncmp(argv[0], "./", 2) == 0 ||
		strncmp(argv[0], "../", 3) == 0)
	{
		execv(argv[0], argv);
	} else {
		size_t cmdlen = strlen(argv[0]);
		char *s = malloc(cmdlen + 6);
		if (!s) err(1, "malloc");
		memcpy(s, "/bin/", 5);
		memcpy(s + 5, argv[0], cmdlen + 1);
		execv(s, argv);
		free(s);
	}
}

void run_args(int argc, char **argv, struct redir *redir) {
	if (!*argv) return;

	/* "special" commands that can't be handled in a subprocess */
	if (!strcmp(argv[0], "mount")) {
		if (argc < 3) {
			fprintf(stderr, "mount: not enough arguments\n");
			return;
		}
		MOUNT_AT("/") {
			fs_dirinject(argv[1]);
		}
		MOUNT_AT(argv[1]) {
			run_args(argc - 2, argv + 2, redir);
			exit(1);
		}
		return;
	} else if (!strcmp(argv[0], "shadow")) {
		if (argc < 2) {
			fprintf(stderr, "shadow: missing path\n");
		} else {
			_sys_mount(HANDLE_NULLFS, argv[1], strlen(argv[1]));
		}
	} else if (!strcmp(argv[0], "procmnt")) {
		if (argc < 2) {
			fprintf(stderr, "procmnt: missing mountpoint\n");
			return;
		}
		_sys_mount(HANDLE_PROCFS, argv[1], strlen(argv[1]));
		/*
		if (!(3 <= argc && !strcmp(argv[2], "raw"))) {
			if (!mount_at("/")) {
				fs_dirinject(argv[1]);
				exit(1);
			}
		}
		*/
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
		printf("%lu ns (assuming 3GHz)\n", time / div);
		return;
	} else if (!strcmp(argv[0], "exit")) {
		exit(0);
	} else if (!strcmp(argv[0], "getpid")) {
		printf("my\t%d\nparent\t%d\n", getpid(), getppid());
		return;
	}

	if (fork()) {
		_sys_await();
		return;
	}

	if (redir && redir->stdout) {
		FILE *f = fopen(redir->stdout, redir->append ? "a" : "w");
		if (!f) {
			err(1, "couldn't open %s for redirection", redir->stdout);
		}

		/* a workaround for file offsets not being preserved across exec()s.
		 * TODO document that weird behaviour of exec() */
		hid_t p[2];
		if (_sys_pipe(p, 0) < 0) {
			errx(1, "couldn't create redirection pipe");
		}
		if (!_sys_fork(FORK_NOREAP, NULL)) {
			/* the child forwards data from the pipe to the file */
			const size_t buflen = 512;
			char *buf = malloc(buflen);
			if (!buf) err(1, "when redirecting");
			close(p[1]);
			for (;;) {
				long len = _sys_read(p[0], buf, buflen, 0);
				if (len < 0) exit(0);
				fwrite(buf, 1, len, f);
				if (ferror(f)) exit(0);
			}
		}

		fclose(f);
		close(p[0]);
		if (_sys_dup(p[1], 1, 0) < 0) {
			errx(1, "dup() failed when redirecting");
		}
	}

	for (struct builtin *iter = builtins; iter->name; iter++) {
		if (!strcmp(argv[0], iter->name)) {
			setprogname(argv[0]);
			iter->fn(argc, argv);
			exit(0);
		}
	}

	execp(argv);
	if (errno == EINVAL) {
		errx(1, "%s isn't a valid executable", argv[0]);
	} else {
		errx(1, "unknown command: %s", argv[0]);
	}
}

static void run(char *cmd) {
#define ARGV_MAX 16
	char *argv[ARGV_MAX];
	struct redir redir;
	int argc = parse(cmd, argv, ARGV_MAX, &redir);
	if (argc < 0) {
		warn("parsing error");
	} else {
		run_args(argc, argv, &redir);
	}
}

int main(int argc, char **argv) {
	static char buf[256];
	FILE *f = stdin;

	if (argc > 1) {
		f = fopen(argv[1], "r");
		if (!f) {
			err(1, "couldn't open %s", argv[1]);
			return 1;
		}
	}

	for (;;) {
		if (f == stdin) {
			printf("%s $ ", getcwd(buf, sizeof buf));
		}
		if (!fgets(buf, 256, f)) {
			return 0;
		}
		run(buf);
	}
}

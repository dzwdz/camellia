#include "builtins.h"
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main();

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


// TODO fgets
static int readline(char *buf, size_t max) {
	char c = '\0';
	size_t pos = 0;
	while (pos < (max-1) && c != '\n' && fread(&c, 1, 1, stdin))
		buf[pos++] = c;
	buf[pos++] = '\0';
	return pos;
}

static void execp(const char *cmd) {
	if (*cmd == '/') {
		execv(cmd, NULL);
		return;
	}

	size_t cmdlen = strlen(cmd);
	char *s = malloc(cmdlen);
	if (!s) {
		printf("sh: out of memory.\n");
		exit(1);
	}
	memcpy(s, "/bin/", 5);
	memcpy(s + 5, cmd, cmdlen + 1);

	char *argv[] = {(char*)cmd, NULL};

	execv(s, argv);
	free(s);
}

static void run(char *cmd) {
	char *args, *redir;
	cmd = strtrim(cmd);
	if (!*cmd) return;

	redir = strtrim(strsplit(cmd, '>'));
	cmd = strtrim(cmd);
	args = strtrim(strsplit(cmd, 0));

	/* "special" commands that can't be handled in a subprocess */
	if (!strcmp(cmd, "shadow")) {
		// TODO process groups
		_syscall_mount(-1, args, strlen(args));
		return;
	} else if (!strcmp(cmd, "exit")) {
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

	if (!strcmp(cmd, "echo")) {
		printf("%s\n", args);
	} else if (!strcmp(cmd, "fork")) {
		main();
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
	} else {
		execp(cmd);
		if (errno == EINVAL) {
			printf("%s isn't a valid executable\n", cmd);
		} else {
			printf("unknown command: %s\n", cmd);
		}
	}
	exit(0); /* kills the subprocess */
}


int main(void) {
	static char buf[256];
	for (;;) {
		printf("$ ");
		readline(buf, 256);
		if (feof(stdin)) return 0;
		run(buf);
	}
}

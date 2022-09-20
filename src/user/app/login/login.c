#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/fs/misc.h>

static const char *shell = "/bin/shell";

static void cutspace(char *s) {
	for (; *s; s++) {
		if (isspace(*s)) {
			*s = '\0';
			break;
		}
	}
}

static bool segcmp(const char *path, int idx, const char *s2) {
	if (idx < 0) return false;
	while (idx > 0) {
		if (*path == '\0') return false;
		if (*path == '/') idx--;
		path++;
	}
	/* path is at the start of the selected segment */
	while (*s2 && *path++ == *s2++);
	return (*path == '\0' || *path == '/') && *s2 == '\0';
}

static void drv(const char *user) {
	char *buf = malloc(PATH_MAX);
	for (;;) {
		struct ufs_request req;
		handle_t reqh = _syscall_fs_wait(buf, PATH_MAX, &req);
		if (reqh < 0) break;

		switch (req.op) {
			case VFSOP_OPEN:
				/* null terminate for segcmp */
				if (req.len == PATH_MAX) {
					_syscall_fs_respond(reqh, NULL, -1, 0);
					break;
				}
				buf[req.len] = '\0';

				if (segcmp(buf, 1, "Users") && segcmp(buf, 2, user)) { //  /Users/$user/**
					forward_open(reqh, buf, req.len, req.flags);
				} else if (segcmp(buf, 1, "Users") && segcmp(buf, 3, "private")) { //  /Users/*/private/**
					_syscall_fs_respond(reqh, NULL, -EACCES, 0);
				} else {
					forward_open(reqh, buf, req.len, req.flags | OPEN_RO);
				}
				break;

			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	free(buf);
}

static void trylogin(const char *user) {
	if (strcmp(user, "root") != 0) {
		char buf[128];
		snprintf(buf, sizeof buf, "/Users/%s/", user);
		if (chdir(buf) < 0) {
			printf("no such user: %s\n", user);
			return;
		}
		MOUNT_AT("/") { drv(user); }
	}

	execv(shell, NULL);
	fprintf(stderr, "login: couldn't launch %s\n", shell);
	exit(1);
}

int main(void) {
	char user[64];
	printf("\nCamellia\n");
	for (;;) {
		printf("login: ");
		fgets(user, sizeof user, stdin);
		if (ferror(stdin)) return -1;

		cutspace(user);
		if (user[0]) trylogin(user);
	}
}

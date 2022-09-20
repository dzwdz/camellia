#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/compat.h>
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

bool segcmp(const char *path, int idx, const char *s2) {
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
	struct ufs_request res;
	char *buf = malloc(PATH_MAX);
	while (!c0_fs_wait(buf, PATH_MAX, &res)) {
		switch (res.op) {
			handle_t h;
			case VFSOP_OPEN:
				/* null terminate */
				if (res.len == PATH_MAX) {
					c0_fs_respond(NULL, -1, 0);
					break;
				}
				buf[res.len] = '\0';

				// TODO use forward_open

				if (segcmp(buf, 1, "Users") && segcmp(buf, 2, user)) {
					// allow full rw access to /Users/$user/**
					h = _syscall_open(buf, res.len, res.flags);
				} else if (segcmp(buf, 1, "Users") && segcmp(buf, 3, "private")) {
					// disallow access to /Users/*/private/**
					h = -EACCES;
				} else {
					// allow ro access to everything else
					h = _syscall_open(buf, res.len, res.flags | OPEN_RO);
				}
				c0_fs_respond(NULL, h, FSR_DELEGATE);
				break;

			default:
				c0_fs_respond(NULL, -1, 0);
				break;
		}
	}
}

static void trylogin(const char *user) {
	if (strcmp(user, "root")) {
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

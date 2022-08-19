#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <ctype.h>
#include <errno.h>
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

static bool accesscheck(const char *path) {
	const char *prefix = "/Users/";
	if (strlen(path) < strlen(prefix) || memcmp(path, prefix, strlen(prefix)))
		return true; /* not an user dir - access allowed */
	path += strlen(prefix);

	/* skip username */
	path = strchr(path, '/');
	if (!path) return true;
	path++;

	/* inside an user dir */
	const char *private = "private/";
	return strlen(path) < strlen(private) || memcmp(path, private, strlen(private));
}

static void drv(const char *prefix) {
	struct fs_wait_response res;
	size_t prefixlen = strlen(prefix);
	char buf[128];
	while (!c0_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			handle_t h;
			case VFSOP_OPEN:
				if (res.len == sizeof buf) {
					c0_fs_respond(NULL, -1, 0);
					break;
				}
				buf[res.len] = '\0';

				if (res.len >= prefixlen && !memcmp(prefix, buf, prefixlen)) {
					h = _syscall_open(buf, res.len, res.flags);
				} else if (accesscheck(buf)) {
					h = _syscall_open(buf, res.len, res.flags | OPEN_RO);
				} else {
					h = -EACCES;
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
		MOUNT_AT("/") { drv(buf); }
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

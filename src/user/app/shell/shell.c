#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void cmd_cat_ls(const char *args, bool ls) {
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
		printf("couldn't open.\n");
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

static void cmd_hexdump(const char *args) {
	static uint8_t buf[512];
	int fd, len;

	fd = _syscall_open(args, strlen(args), 0);
	if (fd < 0) {
		printf("couldn't open.\n");
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

static void cmd_touch(const char *args) {
	int fd = _syscall_open(args, strlen(args), OPEN_CREATE);
	if (fd < 0) {
		printf("couldn't create file.\n");
		return;
	}
	close(fd);
}

int main(void) {
	static char buf[256];
	int level = 0;
	char *cmd, *args, *redir;

	for (;;) {
		printf("%x$ ", level);

		readline(buf, 256);
		if (feof(stdin))
			return 0;

		cmd = strtrim(buf);
		if (!*cmd) continue;

		redir = strtrim(strsplit(cmd, '>'));
		cmd = strtrim(cmd);
		args = strtrim(strsplit(cmd, 0));

		/* "special" commands that can't be handled in a subprocess */
		if (!strcmp(cmd, "shadow")) {
			_syscall_mount(-1, args, strlen(args));
			continue;
		} else if (!strcmp(cmd, "exit")) {
			return 0;
		} else if (!strcmp(cmd, "fork")) {
			if (!fork()) level++;
			else _syscall_await();
			continue;
		}

		if (!fork()) {
			if (redir && !freopen(redir, "w", stdout)) {
				// TODO stderr
				exit(0);
			}

			if (!strcmp(cmd, "echo")) {
				printf("%s\n", args);
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
				char *binname = cmd;
				if (*cmd != '/') {
					size_t cmdlen = strlen(cmd);
					binname = malloc(cmdlen);
					if (!binname) {
						printf("sh: out of memory.\n");
						exit(1);
					}
					memcpy(binname, "/bin/", 5);
					memcpy(binname + 5, cmd, cmdlen + 1);
				}

				execv(binname, NULL);
				if (errno == EINVAL) {
					printf("%s isn't a valid executable\n", cmd);
				} else {
					printf("unknown command: %s\n", cmd);
				}

				if (binname != cmd)
					free(binname);
			}
			exit(0);
		} else {
			_syscall_await();
		}
	}
}

#include <camellia/path.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/elfload.h>

int errno = 0;

int fork(void) {
	return _syscall_fork(0, NULL);
}

int close(handle_t h) {
	return _syscall_close(h);
}

_Noreturn void exit(int c) {
	_syscall_exit(c);
}

int execv(const char *path, char *const argv[]) {
	FILE *file = fopen(path, "r");
	char hdr[4] = {0};
	if (!file)
		return -1;

	fread(hdr, 1, 4, file);
	fseek(file, 0, SEEK_SET);

	if (!memcmp("\x7f""ELF", hdr, 4)) {
		elf_execf(file, (void*)argv, NULL);
		fclose(file);
	} else if (!memcmp("#!", hdr, 2)) {
		char buf[256];
		fseek(file, 2, SEEK_SET);
		if (fgets(buf, sizeof buf, file)) {
			const char *argv [] = {buf, path, NULL};
			char *endl = strchr(buf, '\n');
			if (endl) *endl = '\0';
			execv(buf, (void*)argv);
		}
	}

	errno = EINVAL;
	return -1;
}

_Noreturn void abort(void) {
	_syscall_exit(1);
}


static char *cwd = NULL, *cwd2 = NULL;
static size_t cwdcapacity = 0;

int chdir(const char *path) {
	handle_t h;
	char *tmp;
	size_t len = absolutepath(NULL, path, 0) + 1; /* +1 for the trailing slash */
	if (cwdcapacity < len) {
		cwdcapacity = len;
		if (cwd) {
			cwd = realloc(cwd, len);
			cwd2 = realloc(cwd2, len);
		} else {
			cwd = malloc(len);
			cwd[0] = '/';
			cwd[1] = '\0';
			cwd2 = malloc(len);
			cwd2[0] = '/';
			cwd2[1] = '\0';
		}
	}
	absolutepath(cwd2, path, cwdcapacity);
	len = strlen(cwd2);
	if (cwd2[len - 1] != '/') {
		cwd2[len] = '/';
		cwd2[len + 1] = '\0';
	}

	h = _syscall_open(cwd2, strlen(cwd2), 0);
	if (h < 0) {
		errno = ENOENT;
		return -1;
	}
	_syscall_close(h);

	tmp  = cwd;
	cwd  = cwd2;
	cwd2 = tmp;
	return 0;
}

char *getcwd(char *buf, size_t size) {
	const char *realcwd = cwd ? cwd : "/";
	// TODO bounds checking
	memcpy(buf, realcwd, strlen(realcwd) + 1);
	return buf;
}

size_t absolutepath(char *out, const char *in, size_t size) {
	const char *realcwd = cwd ? cwd : "/";
	size_t len, pos = 0;
	if (!in) return strlen(realcwd);

	if (!(in[0] == '/')) {
		len = strlen(realcwd);
		if (pos + len <= size && out != realcwd)
			memcpy(out + pos, realcwd, len);
		pos += len;

		if (realcwd[len - 1] != '/') {
			if (pos + 1 <= size) out[pos] = '/';
			pos++;
		}
	}

	len = strlen(in);
	if (pos + len <= size)
		memcpy(out + pos, in, len);
	pos += len;

	if (pos <= size) {
		pos = path_simplify(out, out, pos);
		if (pos > 0) out[pos] = '\0';
	}

	if (pos + 1 <= size) out[pos] = '\0';
	pos++;

	return pos;
}

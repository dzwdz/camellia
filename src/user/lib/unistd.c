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
_Noreturn void _exit(int c) { exit(c); };

// TODO unlink
int unlink(const char *path) {
	(void)path;
	errno = ENOSYS;
	return -1;
}

// TODO isatty
int isatty(int fd) {
	return fd <= 2 ? 1 : 0;
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


static const char *__initialcwd;
static char *cwd = NULL, *cwd2 = NULL;
static size_t cwdcapacity = 0;

static const char *getrealcwd(void) {
	/* __initialcwd can't just be initialized with "/" because ld has seemingly
	 * started to revolt against humanity and not process half the relocations
	 * it sees. */
	if (cwd) return cwd;
	if (__initialcwd) return __initialcwd;
	return "/";
}

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
			size_t initlen = strlen(__initialcwd) + 1;
			if (len < initlen)
				len = initlen;
			cwd = malloc(initlen);
			cwd2 = malloc(initlen);
			memcpy(cwd, __initialcwd, initlen);
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

char *getcwd(char *buf, size_t capacity) {
	const char *realcwd = getrealcwd();
	size_t len = strlen(realcwd) + 1;
	if (capacity < len) {
		errno = capacity == 0 ? EINVAL : ERANGE;
		return NULL;
	}
	memcpy(buf, realcwd, len);
	return buf;
}

size_t absolutepath(char *out, const char *in, size_t size) {
	const char *realcwd = getrealcwd();
	size_t len, pos = 0;
	_klogf("realcwd == %x\n", (long)__initialcwd);
	if (!in) return strlen(realcwd) + 1;

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

void __setinitialcwd(const char *s) {
	__initialcwd = s;
}

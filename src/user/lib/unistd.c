#include <bits/panic.h>
#include <camellia.h>
#include <camellia/path.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elfload.h>

int errno = 0;
char **environ = {NULL};

int fork(void) {
	return _sys_fork(0, NULL);
}

pid_t vfork(void) {
	// TODO vfork is implemented improperly and will break stuff
	return _sys_fork(0, NULL);
}

int close(hid_t h) {
	return _sys_close(h);
}

_Noreturn void exit(int c) {
	_sys_exit(c);
}
_Noreturn void _exit(int c) { exit(c); };

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize) {
	(void)path; (void)buf; (void)bufsize;
	errno = ENOSYS;
	return -1;
}

int link(const char *path1, const char *path2) {
	(void)path1; (void)path2;
	errno = ENOSYS;
	return -1;
}

int unlink(const char *path) {
	hid_t h = camellia_open(path, OPEN_WRITE);
	if (h < 0) return errno = -h, -1;
	long ret = _sys_remove(h);
	if (ret < 0) return errno = -ret, -1;
	return 0;
}

int symlink(const char *path1, const char *path2) {
	(void)path1; (void)path2;
	errno = ENOSYS;
	return -1;
}

// TODO isatty
int isatty(int fd) {
	return fd <= 2 ? 1 : 0;
}


int execv(const char *path, char *const argv[]) {
	return execve(path, argv, NULL);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
	FILE *file = fopen(path, "e");
	char hdr[4] = {0};
	if (!file)
		return -1;

	fread(hdr, 1, 4, file);
	fseek(file, 0, SEEK_SET);

	if (!memcmp("\x7f""ELF", hdr, 4)) {
		elf_execf(file, (void*)argv, (void*)envp);
		fclose(file);
	} else if (!memcmp("#!", hdr, 2)) {
		char buf[256];
		fseek(file, 2, SEEK_SET);
		if (fgets(buf, sizeof buf, file)) {
			const char *argv [] = {buf, path, NULL};
			char *endl = strchr(buf, '\n');
			if (endl) *endl = '\0';
			execve(buf, (void*)argv, envp);
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
	hid_t h;
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

	/* check if exists */
	h = camellia_open(cwd2, OPEN_READ);
	if (h < 0) return errno = ENOENT, -1;
	close(h);

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

uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

int chown(const char *path, uid_t owner, gid_t group) {
	(void)path; (void)owner; (void)group;
	errno = ENOSYS;
	return -1;
}

int setpgid(pid_t pid, pid_t pgid) {
	(void)pid; (void)pgid;
	__libc_panic("unimplemented");
}

pid_t tcgetpgrp(int fd) {
	(void)fd;
	__libc_panic("unimplemented");
}

int tcsetpgrp(int fd, pid_t pgrp) {
	(void)fd; (void)pgrp;
	__libc_panic("unimplemented");
}

pid_t getpgrp(void) {
	__libc_panic("unimplemented");
}

pid_t getpid(void) {
	return _sys_getpid();
}

pid_t getppid(void) {
	return _sys_getppid();
}

int getgroups(int size, gid_t list[]) {
	(void)size; (void)list;
	__libc_panic("unimplemented");
}

ssize_t read(int fd, void *buf, size_t count) {
	(void)fd; (void)buf; (void)count;
	__libc_panic("unimplemented");
}

ssize_t write(int fd, const void *buf, size_t count) {
	(void)fd; (void)buf; (void)count;
	__libc_panic("unimplemented");
}

int pipe(int pipefd[2]) {
	(void)pipefd;
	__libc_panic("unimplemented");
}

int dup2(int oldfd, int newfd) {
	(void)oldfd; (void)newfd;
	__libc_panic("unimplemented");
}

size_t absolutepath(char *out, const char *in, size_t size) {
	const char *realcwd = getrealcwd();
	size_t len, pos = 0;
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
		if (pos == 0) return 0;
	}

	if (pos + 1 <= size) out[pos] = '\0';
	pos++;

	return pos;
}

void __setinitialcwd(const char *s) {
	__initialcwd = s;
}

static void intr_null(void) { }

extern void (*volatile _intr)(void);
void intr_set(void (*fn)(void)) {
	_intr = fn ? fn : intr_null;
}

void intr_default(void) {
	exit(-1);
}

/** Functions from unistd.h that are either just a syscall or a return. */

#include <camellia/syscalls.h>
#include <errno.h>
#include <unistd.h>

int errno = 0;
static char *_environ[] = {NULL};
char **environ = _environ;
const char *__initialcwd = NULL;

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
_Noreturn void _exit(int c) {
	exit(c);
};

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

int symlink(const char *path1, const char *path2) {
	(void)path1; (void)path2;
	errno = ENOSYS;
	return -1;
}

// TODO isatty
int isatty(int fd) {
	return fd <= 2 ? 1 : 0;
}

uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

int access(const char *path, int mode) {
	// TODO impl access()
	(void)path; (void)mode;
	return 0;
}

int chown(const char *path, uid_t owner, gid_t group) {
	(void)path; (void)owner; (void)group;
	errno = ENOSYS;
	return -1;
}

int setpgid(pid_t pid, pid_t pgid) {
	(void)pid; (void)pgid;
	return errno = ENOSYS, -1;
}

pid_t tcgetpgrp(int fd) {
	(void)fd;
	return errno = ENOSYS, -1;
}

int tcsetpgrp(int fd, pid_t pgrp) {
	(void)fd; (void)pgrp;
	return errno = ENOSYS, -1;
}

pid_t getpid(void) {
	return _sys_getpid();
}

pid_t getppid(void) {
	return _sys_getppid();
}

ssize_t read(int fd, void *buf, size_t count) {
	// TODO real file descriptor emulation - store offsets
	// TODO errno
	return _sys_read(fd, buf, count, -1);
}

ssize_t write(int fd, const void *buf, size_t count) {
	// TODO real file descriptor emulation - store offsets
	return _sys_write(fd, buf, count, -1, 0);
}

int pipe(int pipefd[2]) {
	// TODO pipe buffering
	int ret = _sys_pipe(pipefd, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return 0;
}

int dup2(int oldfd, int newfd) {
	int ret = _sys_dup(oldfd, newfd, 0);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

unsigned int sleep(unsigned int seconds) {
	_sys_sleep(seconds * 1000);
	return 0;
}

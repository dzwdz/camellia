#pragma once
#include <camellia/types.h> // TODO only needed because of hid_t
#include <sys/types.h>
#include <getopt.h>

// TODO custom stdint.h, ssize_t doesn't belong here
typedef long long ssize_t;

extern char **environ;

int fork(void);
pid_t vfork(void);
int close(hid_t h);
_Noreturn void _exit(int);

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize);
int link(const char *path1, const char *path2);
int unlink(const char *path);
int rmdir(const char *path);
int symlink(const char *path1, const char *path2);
int isatty(int fd);

int execv(const char *path, char *const argv[]);
int execvp(const char *path, char *const argv[]);
int execvpe(const char *path, char *const argv[], char *const envp[]);
int execve(const char *path, char *const argv[], char *const envp[]);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);

uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);

int access(const char *path, int mode);
int chown(const char *path, uid_t owner, gid_t group);

int setpgid(pid_t pid, pid_t pgid);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);

int getgroups(int size, gid_t list[]);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int pipe(int pipefd[2]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);

unsigned int sleep(unsigned int seconds);

/* Converts a relative path to an absolute one, simplifying it if possible.
 * If in == NULL - return the length of cwd. Doesn't include the trailing slash,
 *                 except for the root dir. Includes the null byte.
 * If size isn't enough to fit the path, returns the amount of bytes needed to fit
 * it, including the null byte.
 *
 * Note that some errors are only detected if *out != NULL, so you must check the return
 * value twice.
 * @return 0 on failure, length of the path otherwise */
size_t absolutepath(char *out, const char *in, size_t size);

// TODO put in an internal libc header
void __setinitialcwd(const char *c);

void intr_set(void (*fn)(void));
void intr_default(void);

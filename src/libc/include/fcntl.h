#pragma once

#define F_SETFL 1
#define F_GETFL 2
#define F_DUPFD 3
#define F_SETFD 4

#define FD_CLOEXEC 1

#define O_APPEND 0
#define O_CREAT 0
#define O_EXCL 0
#define O_NONBLOCK 0
#define O_TRUNC 0
#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3

#define R_OK 1
#define W_OK 2
#define X_OK 4

/* it can either take an additonal mode_t argument or none */
int open(const char *path, int flags, ...);
int fcntl(int fd, int cmd, ...);

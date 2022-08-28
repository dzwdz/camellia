#pragma once
#include <errno.h> // only for ENOSYS

#define SIGHUP 0
#define SIGINT 0
#define SIGQUIT 0
#define SIGWINCH 0
#define SIG_DFL 0
#define SIG_ERR 0
#define SIG_IGN 0

typedef int sig_atomic_t;

static inline int signal(int sig, void (*func)(int)) {
	(void)sig; (void)func;
	errno = ENOSYS;
	return SIG_ERR;
}

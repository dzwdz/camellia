#pragma once
#include <bits/panic.h>
#include <sys/types.h>
#include <errno.h> // only for ENOSYS

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGFPE 8
#define SIGKILL 9
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15

#define SIGCONT 16
#define SIGPIPE 17
#define SIGTSTP 18
#define SIGTTIN 19
#define SIGTTOU 20
#define SIGWINCH 21
#define SIGCHLD 22

// idk
#define NSIG 64

#define SIG_DFL 0
#define SIG_ERR 0
#define SIG_IGN 0
#define SIG_SETMASK 0

typedef int sig_atomic_t;
typedef struct {} sigset_t;
typedef struct {} siginfo_t;
extern const char *const sys_siglist[];

struct sigaction {
	void (*sa_handler)(int);
	void (*sa_sigaction)(int, siginfo_t *, void *);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

static inline int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
	(void)sig; (void)act; (void)oldact;
	__libc_panic("unimplemented");
}

static inline int sigemptyset(sigset_t *set) {
	(void)set;
	__libc_panic("unimplemented");
}

static inline int sigfillset(sigset_t *set) {
	(void)set;
	__libc_panic("unimplemented");
}

static inline int sigprocmask(int how, const sigset_t *set, const sigset_t *oldset) {
	(void)how; (void)set; (void)oldset;
	__libc_panic("unimplemented");
}

static inline int sigsuspend(const sigset_t *mask) {
	(void)mask;
	__libc_panic("unimplemented");
}

static inline int signal(int sig, void (*func)(int)) {
	(void)sig; (void)func;
	__libc_panic("unimplemented");
}

static inline int kill(pid_t pid, int sig) {
	(void)pid; (void)sig;
	__libc_panic("unimplemented");
}

static inline int raise(int sig) {
	(void)sig;
	__libc_panic("unimplemented");
}

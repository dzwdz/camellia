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

#define NSIG 32

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

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, const sigset_t *oldset);
int sigsuspend(const sigset_t *mask);
int signal(int sig, void (*func)(int));
int kill(pid_t pid, int sig);
int raise(int sig);

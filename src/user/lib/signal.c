#include <bits/panic.h>
#include <signal.h>

const char *const sys_siglist[] = {
	NULL,
	"SIGHUP", /* 1 */
	"SIGINT", /* 2 */
	"SIGQUIT", /* 3 */
	"SIGILL", /* 4 */
	"SIGTRAP", /* 5 */
	"SIGABRT", /* 6 */
	"SIGFPE", /* 8 */
	"SIGKILL", /* 9 */
	"SIGSEGV", /* 11 */
	"SIGPIPE", /* 13 */
	"SIGALRM", /* 14 */
	"SIGTERM", /* 15 */
	"SIGCONT", /* 16 */
	"SIGPIPE", /* 17 */
	"SIGTSTP", /* 18 */
	"SIGTTIN", /* 19 */
	"SIGTTOU", /* 20 */
	"SIGWINCH", /* 21 */
	"SIGCHLD", /* 22 */
};

static struct sigaction sigaction_default = {};
static struct sigaction *sigaction_current[] = {
	NULL,
	&sigaction_default, /* 1 */
	&sigaction_default, /* 2 */
	&sigaction_default, /* 3 */
	&sigaction_default, /* 4 */
	&sigaction_default, /* 5 */
	&sigaction_default, /* 6 */
	&sigaction_default, /* 8 */
	&sigaction_default, /* 9 */
	&sigaction_default, /* 11 */
	&sigaction_default, /* 13 */
	&sigaction_default, /* 14 */
	&sigaction_default, /* 15 */
	&sigaction_default, /* 16 */
	&sigaction_default, /* 17 */
	&sigaction_default, /* 18 */
	&sigaction_default, /* 19 */
	&sigaction_default, /* 20 */
	&sigaction_default, /* 21 */
	&sigaction_default, /* 22 */
};

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
	const int siglen = sizeof(sigaction_current) / sizeof(sigaction_current[0]);
	if (sig >= siglen) {
		return errno = EINVAL, -1;
	}
	if (oldact) {
		oldact = sigaction_current[sig];
	}
	if (act) {
		if (sig == SIGKILL) {
			return errno = EINVAL, -1;
		}
		sigaction_current[sig] = (void*)act;
	}
	return 0;
}

int sigemptyset(sigset_t *set) {
	(void)set;
	return 0;
}

int sigfillset(sigset_t *set) {
	(void)set;
	return 0;
}

int sigprocmask(int how, const sigset_t *set, const sigset_t *oldset) {
	(void)how; (void)set; (void)oldset;
	__libc_panic("unimplemented");
}

int sigsuspend(const sigset_t *mask) {
	(void)mask;
	__libc_panic("unimplemented");
}

int signal(int sig, void (*func)(int)) {
	(void)sig; (void)func;
	__libc_panic("unimplemented");
}

int kill(pid_t pid, int sig) {
	(void)pid; (void)sig;
	__libc_panic("unimplemented");
}

int raise(int sig) {
	(void)sig;
	__libc_panic("unimplemented");
}

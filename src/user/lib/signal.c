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

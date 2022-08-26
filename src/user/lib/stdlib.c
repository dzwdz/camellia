#include <camellia/syscalls.h>
#include <camellia/flags.h>
#include <errno.h>
#include <string.h>

_Noreturn void abort(void) {
	_syscall_exit(1);
}

int mkstemp(char *template) {
	// TODO randomize template
	handle_t h = _syscall_open(template, strlen(template), OPEN_CREATE);
	if (h < 0) {
		errno = -h;
		return -1;
	}
	// TODO truncate
	return h;
}

// TODO process env
char *getenv(const char *name) {
	(void)name;
	return NULL;
}

// TODO system()
int system(const char *cmd) {
	(void)cmd;
	errno = ENOSYS;
	return -1;
}

#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <string.h>
#include <user/lib/panic.h>

_Noreturn void abort(void) {
	_syscall_exit(1);
}

int mkstemp(char *template) {
	// TODO randomize template
	handle_t h = _syscall_open(template, strlen(template), OPEN_CREATE | OPEN_RW);
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

int abs(int i) {
	return i < 0 ? -i : i;
}

int atoi(const char *s) {
	return strtol(s, NULL, 10);
}

double atof(const char *s) {
	(void)s;
	__libc_panic("unimplemented");
}

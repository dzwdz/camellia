#include <_proc.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <string.h>
#include <user/lib/panic.h>

_Noreturn void abort(void) {
	_sys_exit(1);
}

static const char *progname;
const char *getprogname(void) {
	return progname;
}
void setprogname(const char *pg) {
	progname = pg;
}

void setproctitle(const char *fmt, ...) {
	if (!fmt) {
		strcpy(_libc_psdata, progname);
		return;
	}
	sprintf(_libc_psdata, "%s: ", progname);

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(_libc_psdata + strlen(_libc_psdata), 128, fmt, argp);
	va_end(argp);
}

int mkstemp(char *template) {
	// TODO randomize template
	hid_t h = camellia_open(template, OPEN_CREATE | OPEN_RW);
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

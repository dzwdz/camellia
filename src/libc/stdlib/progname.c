#include <_proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *progname;
const char *getprogname(void) {
	return progname;
}
void setprogname(const char *pg) {
	progname = pg;
	setproctitle(NULL);
}

void setproctitle(const char *fmt, ...) {
	// TODO bounds checking
	if (!fmt) {
		strcpy(_psdata_loc->desc, progname);
		return;
	}
	sprintf(_psdata_loc->desc, "%s: ", progname);

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(_psdata_loc->desc + strlen(_psdata_loc->desc), 128, fmt, argp);
	va_end(argp);
}


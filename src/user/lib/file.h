#pragma once
#include <stdbool.h>

struct _LIBC_FILE {
	int fd;
	long pos;
	bool eof;
	bool error;
};

#pragma once
#include <stdbool.h>

struct _LIBC_FILE {
	int fd;
	int pos;
	bool eof;
	bool error;
};

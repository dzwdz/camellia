#pragma once
#include <stdbool.h>
#include <stdio.h>

struct _LIBC_FILE {
	int fd;
	long pos;
	bool eof;
	bool error;
	int extflags;

	char *readbuf;
	size_t rblen, rbcap;
};

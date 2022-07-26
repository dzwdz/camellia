#pragma once
#include <stdbool.h>
// TODO make opaque
struct libc_file {
	int fd;
	int pos;
	bool eof;
};
typedef struct libc_file libc_file;

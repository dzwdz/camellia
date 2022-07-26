#pragma once
#include <stdbool.h>
// TODO make opaque
struct FILE {
	int fd;
	int pos;
	bool eof;
	bool error;
};
typedef struct FILE FILE;

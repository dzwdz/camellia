#pragma once
#include <stdbool.h>
// TODO make opaque
struct FILE {
	int fd;
	int pos;
	bool eof;
};
typedef struct FILE FILE;

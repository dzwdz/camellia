#pragma once
#include <bits/panic.h>

typedef struct DIR DIR;
struct dirent {
	ino_t d_ino;
	char d_name[256]; /* NAME_MAX + 1 */
};

static inline DIR *opendir(const char *name) {
	__libc_panic("unimplemented");
}

static inline int closedir(DIR *dir) {
	__libc_panic("unimplemented");
}

static inline struct dirent *readdir(DIR *dir) {
	__libc_panic("unimplemented");
}

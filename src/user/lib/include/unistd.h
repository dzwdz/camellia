#pragma once
#include <camellia/types.h> // TODO only needed because of handle_t

int fork(void);
int close(handle_t h);
_Noreturn void exit(int);

int execv(const char *path, char *const argv[]);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);
/* Converts a relative path to an absolute one, simplifying it if possible.
 * If in == NULL - return the length of cwd (without a trailing slash)
 * If size isn't enough to fit the path, returns the amount of bytes needed to fit
 * it, including the null byte.
 * @return 0 on failure, length of the path otherwise */
size_t absolutepath(char *out, const char *in, size_t size);

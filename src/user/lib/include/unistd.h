#pragma once
#include <camellia/types.h> // TODO only needed because of hid_t
#include <getopt.h>

int fork(void);
int close(hid_t h);
_Noreturn void _exit(int);

int unlink(const char *path);
int isatty(int fd);

int execv(const char *path, char *const argv[]);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);
/* Converts a relative path to an absolute one, simplifying it if possible.
 * If in == NULL - return the length of cwd. Doesn't include the trailing slash,
 *                 except for the root dir. Includes the null byte.
 * If size isn't enough to fit the path, returns the amount of bytes needed to fit
 * it, including the null byte.
 *
 * Note that some errors are only detected if *out != NULL, so you must check the return
 * value twice.
 * @return 0 on failure, length of the path otherwise */
size_t absolutepath(char *out, const char *in, size_t size);

// TODO put in an internal libc header
void __setinitialcwd(const char *c);

void intr_set(void (*fn)(void));
void intr_default(void);

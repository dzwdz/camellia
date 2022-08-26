#pragma once
#include <stddef.h>
#include <stdlib.h>

#ifndef NO_MALLOC_H
#include <user/lib/vendor/dlmalloc/malloc.h>
#endif

_Noreturn void abort(void);

int mkstemp(char *template);
char *getenv(const char *name);
int system(const char *cmd);

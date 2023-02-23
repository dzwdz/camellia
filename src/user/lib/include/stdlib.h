#pragma once
#include <stddef.h>
#include <stdlib.h>

#ifndef NO_MALLOC_H
#include <malloc.h>
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

_Noreturn void abort(void);
_Noreturn void exit(int);

const char *getprogname(void);
void setprogname(const char *progname);
void setproctitle(const char *fmt, ...);

int mkstemp(char *template);
char *getenv(const char *name);
int system(const char *cmd);

int abs(int i);

int atoi(const char *s);
double atof(const char *s);

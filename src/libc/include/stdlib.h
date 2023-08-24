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

char *mktemp(char *tmpl);
int mkstemp(char *tmpl);
char *getenv(const char *name);
int system(const char *cmd);

int abs(int i);

int atoi(const char *s);
long atol(const char *s);
double atof(const char *s);

long strtol(const char *restrict s, char **restrict end, int base);
long long strtoll(const char *restrict s, char **restrict end, int base);
unsigned long strtoul(const char *restrict s, char **restrict end, int base);
unsigned long long strtoull(const char *restrict s, char **restrict end, int base);
double strtod(const char *restrict s, char **restrict end);

void qsort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *a, const void *b));
void qsort_r(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *arg);
void* bsearch(const void* key, const void* base_ptr, size_t nmemb, size_t size, int (*compare)(const void*, const void*));

#pragma once
#include <bits/file.h>
#include <stddef.h>

#define EOF (-1)

int printf(const char *fmt, ...);
int snprintf(char *str, size_t len, const char *fmt, ...);

int _klogf(const char *fmt, ...); // for kernel debugging only


extern FILE *const stdin, *const stdout;

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *);
FILE *fdopen(int fd, const char *mode);
FILE *file_clone(const FILE *);
int fclose(FILE *);

size_t fread(void *restrict ptr, size_t size, size_t nitems, FILE *restrict);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict);

int feof(FILE *);
int ferror(FILE *);

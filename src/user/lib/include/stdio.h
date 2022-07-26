#pragma once
#include <bits/file.h>
#include <stddef.h>

int printf(const char *fmt, ...);
int snprintf(char *str, size_t len, const char *fmt, ...);

int _klogf(const char *fmt, ...); // for kernel debugging only


extern FILE *const stdin, *const stdout;

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE*);
FILE *file_clone(const FILE*);
int file_read(FILE*, char *buf, size_t len);
int file_write(FILE*, const char *buf, size_t len);
void file_close(FILE*);

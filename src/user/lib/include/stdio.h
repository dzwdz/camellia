#pragma once
#include <bits/file.h>
#include <stddef.h>

int printf(const char *fmt, ...);
int snprintf(char *str, size_t len, const char *fmt, ...);

int _klogf(const char *fmt, ...); // for kernel debugging only


extern libc_file *const stdin, *const stdout;

libc_file *file_open(const char *path, int flags);
libc_file *file_reopen(libc_file*, const char *path, int flags);
libc_file *file_clone(const libc_file*);
int file_read(libc_file*, char *buf, size_t len);
int file_write(libc_file*, const char *buf, size_t len);
void file_close(libc_file*);

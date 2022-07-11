#pragma once
#include <shared/mem.h>
#include <shared/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <user/lib/malloc.h>

int printf(const char *fmt, ...);
int snprintf(char *str, size_t len, const char *fmt, ...);

int _klogf(const char *fmt, ...); // for kernel debugging only

typedef struct {
	int fd;
	int pos;
	bool eof;
} libc_file;
libc_file *file_open(const char *path, int flags);
libc_file *file_reopen(libc_file*, const char *path, int flags);
int file_read(libc_file*, char *buf, size_t len);
int file_write(libc_file*, const char *buf, size_t len);
void file_close(libc_file*);

extern libc_file *stdin, *stdout;

int fork(void);
int close(handle_t h);

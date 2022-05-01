#pragma once
#include <shared/mem.h>
#include <stdbool.h>
#include <stddef.h>

int printf(const char *fmt, ...);

typedef struct {
	int fd;
	int pos;
	bool eof;
} libc_file;
int file_open(libc_file*, const char *path); // TODO return a libc_file*
int file_read(libc_file*, char *buf, size_t len);
int file_write(libc_file*, const char *buf, size_t len);
void file_close(libc_file*);

extern libc_file __stdin, __stdout;

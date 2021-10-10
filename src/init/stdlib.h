#pragma once
#include <stddef.h>

extern int __tty_fd;

int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
int printf(const char *fmt, ...);

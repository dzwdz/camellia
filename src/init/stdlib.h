#pragma once
#include <shared/mem.h>
#include <stddef.h>

extern int __tty_fd;

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
int printf(const char *fmt, ...);

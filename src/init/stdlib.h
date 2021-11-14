#pragma once
#include <shared/mem.h>
#include <stddef.h>

extern int __tty_fd;

int printf(const char *fmt, ...);

#pragma once
#include <shared/mem.h>
#include <stddef.h>

extern int __stdin, __stdout;

int printf(const char *fmt, ...);

#pragma once
#include <stdarg.h>
#include <stddef.h>

/* note: (partially) tested in the userland tests */

void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);

int snprintf(char *restrict str, size_t len, const char *restrict fmt, ...);
int vsnprintf(char *restrict str, size_t len, const char *restrict fmt, va_list ap);

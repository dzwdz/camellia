#pragma once
#include <shared/mem.h>
#include <strings.h> /* work around bad include in dash */

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s1, const char *s2);

char *strtok(char *restrict s, const char *restrict sep);
char *strtok_r(char *restrict s, const char *restrict sep, char **restrict state);

int strncmp(const char *s1, const char *s2, size_t n);
int strcoll(const char *s1, const char *s2);

char *strstr(const char *s1, const char *s2);

char *strcat(char *restrict dst, const char *restrict src);
char *strcpy(char *restrict s1, const char *restrict s2);
char *strncpy(char *restrict s1, const char *restrict s2, size_t n);
char *strncat(char *restrict dst, const char *restrict src, size_t n);
char *stpncpy(char *restrict dst, const char *restrict src, size_t n);
char *strdup(const char *s);

size_t strnlen(const char *s, size_t len);

char *strerror(int errnum);
char *strsignal(int sig);

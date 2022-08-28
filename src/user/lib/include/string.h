#pragma once
#include <shared/mem.h>

long strtol(const char *restrict s, char **restrict end, int base);
double strtod(const char *restrict s, char **restrict end);
char *strchr(const char *s, int c);

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s1, const char *s2);

char *strtok(char *restrict s, const char *restrict sep);
char *strtok_r(char *restrict s, const char *restrict sep, char **restrict state);

int strncmp(const char *s1, const char *s2, size_t n);
int strcoll(const char *s1, const char *s2);

char *strstr(const char *s1, const char *s2);

char *strcpy(char *restrict s1, const char *restrict s2);

char *strerror(int errnum);

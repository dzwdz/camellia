#pragma once
#include <stddef.h>

#define __NUM2STR(x) #x
#define NUM2STR(x)  __NUM2STR(x)

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

// s1 MUST be a string known at compiletime
#define static_strcmp(s1, s2) \
	memcmp(s1, s2, sizeof(s1))

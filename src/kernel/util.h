#pragma once
#include <stddef.h>

#define __NUM2STR(x) #x
#define NUM2STR(x)  __NUM2STR(x)

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

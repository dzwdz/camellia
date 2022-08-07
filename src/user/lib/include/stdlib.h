#pragma once
#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);

_Noreturn void abort(void);

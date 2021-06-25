#pragma once
#include <stddef.h>

void mem_init();

void *malloc(size_t size);
void free(void *ptr);

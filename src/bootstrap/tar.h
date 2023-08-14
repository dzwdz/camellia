#pragma once
#include <camellia/types.h>
#include <stddef.h>

_Noreturn void tar_driver(void *base);
void *tar_find(const char *path, size_t path_len, void *base, size_t base_len);

#pragma once
#include <shared/types.h>

_Noreturn void tar_driver(void *base);
void *tar_find(const char *path, size_t path_len, void *base, size_t base_len);

#pragma once
#include <stddef.h>

void log_write(const char *buf, size_t len);

// used for static strings
#define log_const(str) log_write(str, sizeof(str) - 1)

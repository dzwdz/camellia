#pragma once
#include <stddef.h>

void serial_preinit(void);
void serial_write(const char *buf, size_t len);
void serial_init(void);

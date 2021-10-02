#pragma once
#include <stddef.h>

char serial_read(void);
void serial_write(const char *buf, size_t len);
void serial_init(void);

#pragma once
#include <stdbool.h>
#include <stddef.h>

bool serial_poll_read(char *c);
void serial_write(const char *buf, size_t len);
void serial_init(void);

#pragma once

#include <stddef.h>

void tty_putchar(char c);
void tty_write(const char *buf, size_t len);
void tty_clear();

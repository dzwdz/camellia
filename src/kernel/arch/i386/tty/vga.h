#pragma once
#include <stddef.h>

void vga_putchar(char c);
void vga_write(const char *buf, size_t len);
void vga_clear();

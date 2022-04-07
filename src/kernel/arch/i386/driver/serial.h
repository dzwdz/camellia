#pragma once
#include <stdbool.h>
#include <stddef.h>

void	serial_init(void);
size_t	serial_read(char *buf, size_t len);
void	serial_write(const char *buf, size_t len);

bool	serial_poll_read(char *c);

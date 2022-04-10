#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool	ps2_ready(void);
void	ps2_recv(uint8_t scancode);
size_t	ps2_read(uint8_t *buf, size_t max_len);

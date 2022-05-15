#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
	char *buf;
	size_t capacity;
	size_t _head, _tail;
} ring_t;

size_t	ring_size(ring_t*);

void	ring_put(ring_t*, void*, size_t);
void	ring_put1b(ring_t*, uint8_t);

size_t	ring_get(ring_t*, void*, size_t);

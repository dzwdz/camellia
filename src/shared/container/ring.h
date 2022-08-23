#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
	char *buf;
	size_t capacity;
	size_t _head, _tail;
} ring_t;

/** Returns amount of bytes stored in the buffer. */
size_t	ring_used(ring_t*);
/** Returns amount of space left in the buffer. */
size_t	ring_avail(ring_t*);

void	ring_put(ring_t*, void*, size_t);
void	ring_put1b(ring_t*, uint8_t);

size_t	ring_get(ring_t*, void*, size_t);

/** Consumes up to `len` bytes, and returns a pointer to the buffer where it's stored.
 * Not thread-safe. */
void*	ring_contig(ring_t*, size_t *len);

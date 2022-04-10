#include <shared/container/ring.h>
#include <stdbool.h>
// TODO use memcpy

static bool at_end(ring_t *r) {
	return  r->_head + 1 == r->_tail
		|| (r->_head + 1 == r->capacity && r->_tail == 0);
}

size_t ring_size(ring_t *r) {
	if (r->_head >= r->_tail)
		return r->_head - r->_tail;
	else
		return r->_head + r->capacity - r->_tail;
}

void ring_put(ring_t *r, void *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		ring_put1b(r, ((uint8_t*)buf)[i]);
}

void ring_put1b(ring_t *r, uint8_t byte) {
	if (at_end(r)) return;
	((uint8_t*)r->buf)[r->_head++] = byte;
	if (r->_head >= r->capacity) r->_head = 0;
}

size_t ring_get(ring_t *r, void *buf, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if (r->_head == r->_tail || at_end(r)) return i;
		((uint8_t*)buf)[i] = ((uint8_t*)r->buf)[r->_tail++];
		if (r->_tail >= r->capacity) r->_tail = 0;
	}
	return len;
}

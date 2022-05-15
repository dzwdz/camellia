#include <shared/container/ring.h>
#include <shared/mem.h>
#include <stdbool.h>

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
	// TODO do something similar to ring_get
	for (size_t i = 0; i < len; i++)
		ring_put1b(r, ((uint8_t*)buf)[i]);
}

void ring_put1b(ring_t *r, uint8_t byte) {
	if (at_end(r)) return;
	r->buf[r->_head++] = byte;
	if (r->_head >= r->capacity) r->_head = 0;
}

size_t ring_get(ring_t *r, void *buf, size_t len) {
	size_t read = 0;
	size_t plen;
	void *pbuf;
	for (size_t i = 0; i < 2; i++) {
		plen = len - read;
		pbuf = ring_contig(r, &plen);
		if (buf) memcpy(buf + read, pbuf, plen);
		read += plen;
	}
	return read;
}

void *ring_contig(ring_t *r, size_t *len) {
	void *ret = &r->buf[r->_tail];
	size_t avail;
	if (r->_head >= r->_tail)
		avail = r->_head - r->_tail;
	else
		avail = r->capacity - r->_tail;

	if (*len > avail)
		*len = avail;

	r->_tail += *len;
	if (r->_tail >= r->capacity) r->_tail = 0;
	return ret;
}

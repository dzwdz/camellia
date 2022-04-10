#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <shared/mem.h>

#define BACKLOG_CAPACITY 64

static volatile uint8_t backlog[BACKLOG_CAPACITY] = {};
static volatile size_t backlog_size = 0;

bool ps2_ready(void) {
	return backlog_size > 0;
}

void ps2_recv(uint8_t s) {
	if (backlog_size >= BACKLOG_CAPACITY) return;
	backlog[backlog_size++] = s;
}

size_t ps2_read(uint8_t *buf, size_t len) {
	if (backlog_size <= len)
		len = backlog_size;
	backlog_size -= len; /* guaranteed to never be < 0 */
	memcpy(buf, (void*)backlog, len);

	/* move rest of buffer back on partial reads */
	// TODO assumes that memcpy()ing into an overlapping buffer is fine, outside spec
	if (backlog_size > 0)
		memcpy((void*)backlog, (void*)backlog + len, backlog_size);

	return len;
}

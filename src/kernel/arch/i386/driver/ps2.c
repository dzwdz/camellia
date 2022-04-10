#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <shared/container/ring.h>
#include <shared/mem.h>

#define BACKLOG_CAPACITY 64
static volatile uint8_t backlog_buf[BACKLOG_CAPACITY];
static volatile ring_t backlog = {(void*)backlog_buf, BACKLOG_CAPACITY, 0, 0};

bool ps2_ready(void) {
	return ring_size((void*)&backlog) > 0;
}

void ps2_recv(uint8_t s) {
	ring_put1b((void*)&backlog, s);
}

size_t ps2_read(uint8_t *buf, size_t len) {
	return ring_get((void*)&backlog, buf, len);
}

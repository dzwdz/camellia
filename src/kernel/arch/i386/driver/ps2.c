#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <shared/mem.h>

#define BACKLOG_CAPACITY 16

/* TODO move away from tty/ */

static volatile uint8_t backlog[BACKLOG_CAPACITY] = {};
static volatile size_t backlog_size = 0;

void ps2_recv(uint8_t s) {
	if (backlog_size >= BACKLOG_CAPACITY) return;
	backlog[backlog_size++] = s;
}

size_t ps2_read(uint8_t *buf, size_t max_len) {
	irq_interrupt_flag(true);
	while (backlog_size == 0)
		asm("hlt" ::: "memory");
	irq_interrupt_flag(false);

	size_t len = backlog_size;
	if (len > max_len) len = max_len;
	backlog_size = 0;
	memcpy(buf, (void*)backlog, len);
	return len;
}

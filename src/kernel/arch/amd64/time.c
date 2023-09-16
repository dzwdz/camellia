#include <kernel/arch/amd64/acpi.h>
#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/interrupts.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

static Proc *scheduled = NULL;

static void *hpet = NULL;
static uint64_t hpet_period;

void hpet_init(void *base) {
	if (hpet) panic_invalid_state();
	hpet = base;

	uint64_t *info = hpet;
	uint64_t *cfg = hpet + 0x10;
	hpet_period = *info >> 32;
	*cfg |= 1<<0; /* enable */
}

uint64_t uptime_ns(void) {
	if (hpet == NULL) panic_invalid_state();
	uint64_t *counter = hpet + 0xF0;
	unsigned __int128 femto = *counter * hpet_period;
	uint64_t nano = femto / 1000000;
	return nano;
}

static void pit_irq(void) {
	Proc *p = scheduled;
	if (p && p->waits4timer.goal < uptime_ns()) {
		scheduled = p->waits4timer.next;
		proc_setstate(p, PS_RUNNING);
	}
}


void timer_schedule(Proc *p, uint64_t time) {
	uint64_t now = uptime_ns();
	/* to prevent leaking the current uptime, saturate instead of overflowing */
	time = now + time;
	if (time < now) {
		time = ~0;
	}

	proc_setstate(p, PS_WAITS4TIMER);
	p->waits4timer.goal = time;

	Proc **slot = &scheduled;
	while (*slot && (*slot)->waits4timer.goal <= time) {
		assert((*slot)->state == PS_WAITS4TIMER);
		slot = &(*slot)->waits4timer.next;
	}
	p->waits4timer.next = *slot;
	*slot = p;
}

void timer_deschedule(Proc *p) {
	assert(p->state == PS_WAITS4TIMER);

	Proc **slot = &scheduled;
	while (*slot && *slot != p)
		slot = &(*slot)->waits4timer.next;
	assert(*slot);
	*slot = p->waits4timer.next;

	proc_setstate(p, PS_RUNNING);
}

void timer_init(void) {
	irq_fn[IRQ_PIT] = pit_irq;
}

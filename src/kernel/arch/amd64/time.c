#include <kernel/arch/amd64/acpi.h>
#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/interrupts.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

static Proc *scheduled = NULL;

static volatile void *hpet = NULL;
static uint64_t hpet_period;

static void hpet_irq(void);

enum {
	/* bitfields */
	HpetIs64Bit = 1<<13,
	HpetHasLegacyMap = 1<<15,

	HpetEnabled = 1<<0,
	HpetUseLegacyMap = 1<<1,

	HpetTimerTrigger = 1<<1, /* true if level triggered */
	HpetTimerEnable = 1<<2,
	HpetTimerPeriodic = 1<<3,
	HpetTimer64 = 1<<5,
};

void hpet_init(void *base) {
	if (hpet) panic_invalid_state();
	hpet = base;

	volatile uint64_t *info = hpet;
	volatile uint64_t *cfg = hpet + 0x10;
	hpet_period = *info >> 32;
	if (!(*info & HpetIs64Bit)) panic_unimplemented();
	if (!(*info & HpetHasLegacyMap)) panic_unimplemented();

	volatile uint64_t *timcfg = hpet + 0x120;
	if (!(*timcfg & HpetTimer64)) panic_unimplemented();
	*timcfg |= HpetTimerTrigger;
	*timcfg &= ~HpetTimerPeriodic;
	*cfg |= HpetEnabled | HpetUseLegacyMap;

	irq_fn[IRQ_HPET] = hpet_irq;
}

static void hpet_sched(void) {
	volatile uint64_t *status = hpet + 0x20;
	volatile uint64_t *timcfg = hpet + 0x120;
	volatile uint64_t *timval = hpet + 0x128;
	if (scheduled) {
		*timval = scheduled->waits4timer.goal;
		*timcfg |= HpetTimerEnable;
	} else {
		*timcfg &= ~HpetTimerEnable;
	}
	*status = ~0;
}

static uint64_t get_counter(void) {
	if (hpet == NULL) panic_invalid_state();
	volatile uint64_t *counter = hpet + 0xF0;
	return *counter;
}

uint64_t uptime_ns(void) {
	unsigned __int128 femto = get_counter() * hpet_period;
	uint64_t nano = femto / 1000000;
	return nano;
}

static void hpet_irq(void) {
	Proc *p = scheduled;
	if (p && p->waits4timer.goal < get_counter()) {
		scheduled = p->waits4timer.next;
		proc_setstate(p, PS_RUNNING);
	}
	hpet_sched();
}


void timer_schedule(Proc *p, uint64_t time) {
	uint64_t now = get_counter();
	/* to prevent leaking the current uptime, saturate instead of overflowing */
	time = now + (time * 1000000 / hpet_period);
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
	hpet_sched(); // TODO can sometimes be skipped
}

void timer_deschedule(Proc *p) {
	assert(p->state == PS_WAITS4TIMER);

	Proc **slot = &scheduled;
	while (*slot && *slot != p)
		slot = &(*slot)->waits4timer.next;
	assert(*slot);
	*slot = p->waits4timer.next;

	proc_setstate(p, PS_RUNNING);
	hpet_sched(); // TODO can sometimes be skipped
}

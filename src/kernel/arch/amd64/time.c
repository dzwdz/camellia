#include <kernel/arch/amd64/time.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

static uint64_t uptime = 0, goal = ~0;
static struct process *scheduled = NULL;

uint64_t uptime_ms(void) { return uptime; }

static void update_goal(void) {
	goal = scheduled ? scheduled->waits4timer.goal : ~(uint64_t)0;
}

void pit_irq(void) {
	// TODO inefficient - getting here executes a lot of code which could just be a few lines of asm
	uptime++;
	if (uptime < goal) return;

	struct process *p = scheduled;
	assert(p);
	scheduled = p->waits4timer.next;
	process_transition(p, PS_RUNNING);
	update_goal();
}

void timer_schedule(struct process *p, uint64_t time) {
	process_transition(p, PS_WAITS4TIMER);
	p->waits4timer.goal = time;

	struct process **slot = &scheduled;
	while (*slot && (*slot)->waits4timer.goal <= time) {
		assert((*slot)->state == PS_WAITS4TIMER);
		slot = &(*slot)->waits4timer.next;
	}
	p->waits4timer.next = *slot;
	*slot = p;
	update_goal();
}

void timer_deschedule(struct process *p) {
	assert(p->state == PS_WAITS4TIMER);

	struct process **slot = &scheduled;
	while (*slot && *slot != p)
		slot = &(*slot)->waits4timer.next;
	assert(*slot);
	*slot = p->waits4timer.next;

	update_goal();
}

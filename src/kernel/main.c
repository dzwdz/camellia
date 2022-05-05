#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/proc.h>
#include <kernel/tests/tests.h>
#include <kernel/util.h>
#include <stdint.h>

void kmain(struct kmain_info info) {
	kprintf("mem...\n");
	mem_init(&info);

	kprintf("tests...\n");
	tests_all();

	kprintf("loading init...\n");
	process_seed(&info);

	kprintf("switching...\n");
	process_switch_any();
}

void shutdown(void) {
	size_t states[PS_LAST] = {0};
	for (struct process *p = process_first; p; p = process_next(p))
		states[p->state]++;

	kprintf("\n\n\nshutting off\n");
	for (size_t i = 0; i < sizeof(states) / sizeof(*states); i++)
		kprintf("state 0x%x: 0x%x\n", i, states[i]);

	mem_debugprint();
	cpu_shutdown();
}

#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/proc.h>
#include <kernel/tests/tests.h>
#include <kernel/util.h>
#include <stdint.h>

_Noreturn static void run_init(struct kmain_info *info) {
	// TODO move all of this to process_seed
	struct process *proc = process_seed();
	void __user *init_base = (userptr_t)0x200000;

	// map the module as rw
	for (uintptr_t off = 0; off < info->init.size; off += PAGE_SIZE)
		pagedir_map(proc->pages, init_base + off, info->init.at + off,
		            true, true);
	proc->regs.eip = init_base;

	kprintf("switching...\n");
	process_switch_any();
}

void kmain(struct kmain_info info) {
	kprintf("mem...\n");
	mem_init(&info);

	kprintf("tests...\n");
	tests_all();

	kprintf("loading init...\n");
	run_init(&info);
}

void shutdown(void) {
	size_t states[PS_LAST] = {0};
	for (struct process *p = process_first; p; p = process_next(p))
		states[p->state]++;
	for (size_t i = 0; i < sizeof(states) / sizeof(*states); i++)
		kprintf("state 0x%x: 0x%x\n", i, states[i]);

	mem_debugprint();
	cpu_shutdown();
}

#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <stdint.h>

static void run_init(struct kmain_info *info) {
	struct process *proc = process_new();
	void *init_base = (void*) 0x200000;

	// map the module as rw
	for (uintptr_t off = 0; off < info->init.size; off += PAGE_SIZE)
		pagedir_map(proc->pages, init_base + off, info->init.at + off,
		            true, true);
	proc->regs.eip = init_base;

	log_const("switching...");
	process_switch(proc);
}

void kmain(struct kmain_info info) {
	log_const("mem...");
	mem_init(&info);

	log_const("loading init...");
	run_init(&info);

	panic();
}

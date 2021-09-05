#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/tests/tests.h>
#include <kernel/util.h>
#include <stdint.h>

static void run_init(struct kmain_info *info) {
	struct process *proc = process_seed();
	user_ptr init_base = 0x200000;

	// map the module as rw
	for (uintptr_t off = 0; off < info->init.size; off += PAGE_SIZE)
		pagedir_map(proc->pages, init_base + off, info->init.at + off,
		            true, true);
	proc->regs.eip = init_base;

	tty_const("switching...");
	process_switch(proc);
}

void kmain(struct kmain_info info) {
	tty_const("mem...");
	mem_init(&info);

	tty_const("tests...");
	tests_all();

	tty_const("loading init...");
	run_init(&info);

	panic();
}

#include <arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

void r3_test();

void kmain() {
	tty_const("mem...");
	mem_init();

	tty_const("creating process...");
	struct process *proc = process_new(r3_test);
	tty_const("switching...");
	process_switch(proc);
}

void r3_test() {
	tty_const("ok");
	asm("cli");
	panic();
}

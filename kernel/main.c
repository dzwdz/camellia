#include <kernel/gdt.h>
#include <kernel/mem.h>
#include <kernel/proc.h>
#include <kernel/tty.h>
#include <platform/sysenter.h>

void r3_test();

void kmain()
{
	tty_clear();
	gdt_init();
	sysenter_setup();
	mem_init();

	tty_const("creating process...");
	struct process *proc = process_new(r3_test);
	tty_const("switching...");
	process_switch(proc);
}

void r3_test() {
	tty_const("ok");
	asm("cli"); // privileged instruction, should cause a GP
	tty_const(" this shouldn't happen");
	for (;;) {}
}

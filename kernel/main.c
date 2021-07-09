#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/tty.h>
#include <platform/asm.h>

void r3_test();

void kmain()
{
	tty_clear();
	tty_const("gdt...");
	gdt_init();
	tty_const("idt...");
	idt_init();
	tty_const("sysenter...");
	sysenter_setup();
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

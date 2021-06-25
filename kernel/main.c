#include <kernel/gdt.h>
#include <kernel/tty.h>
#include <platform/sysenter.h>

extern void stack_top;

void r3_test();

void kmain()
{
	tty_clear();
	gdt_init();
	sysenter_setup();

	tty_const("jumping to ring3...");
	sysexit(r3_test, &stack_top);
}

void r3_test() {
	tty_const("ok");
	asm("cli"); // privileged instruction, should cause a GP
	tty_const(" this shouldn't happen");
	for (;;) {}
}

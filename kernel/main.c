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

	tty_write("user...", 7);
	sysexit(r3_test, &stack_top);
}

void r3_test() {
	tty_write("in ring3", 8);
	asm("cli"); // privileged instruction, should cause a GP
	tty_write(" oh no", 6); // shouldn't happen
	for (;;) {}
}

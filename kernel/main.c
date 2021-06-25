#include <kernel/gdt.h>
#include <kernel/tty.h>

void kmain()
{
	tty_clear();
	gdt_init();
}

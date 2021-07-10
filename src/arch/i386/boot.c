#include <arch/i386/gdt.h>
#include <arch/i386/interrupts/idt.h>
#include <arch/i386/sysenter.h>
#include <arch/i386/tty.h>
#include <kernel/main.h>

void kmain_early() {
	tty_clear();
	tty_const("gdt...");
	gdt_init();
	tty_const("idt...");
	idt_init();
	tty_const("sysenter...");
	sysenter_setup();
	kmain();
}

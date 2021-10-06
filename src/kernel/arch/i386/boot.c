#include <kernel/arch/generic.h>
#include <kernel/arch/i386/ata.h>
#include <kernel/arch/i386/boot.h>
#include <kernel/arch/i386/gdt.h>
#include <kernel/arch/i386/interrupts/idt.h>
#include <kernel/arch/i386/multiboot.h>
#include <kernel/main.h>
#include <kernel/panic.h>

void kmain_early(struct multiboot_info *multiboot) {
	struct kmain_info info;

	// setup some basic stuff
	tty_init();
	tty_const("gdt...");
	gdt_init();
	tty_const("idt...");
	idt_init();
	tty_const("ata...");
	ata_init();

	{ // find the init module
		struct multiboot_mod *module = &multiboot->mods[0];
		if (multiboot->mods_count < 1) {
			tty_const("can't find init! ");
			panic_invalid_state(); // no init
		}
		info.init.at   = module->start;
		info.init.size = module->end - module->start;
	}

	kmain(info);
}

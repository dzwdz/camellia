#include <arch/generic.h>
#include <arch/i386/gdt.h>
#include <arch/i386/interrupts/idt.h>
#include <arch/i386/multiboot.h>
#include <arch/i386/sysenter.h>
#include <arch/i386/tty.h>
#include <kernel/main.h>
#include <kernel/panic.h>

void module_test(struct multiboot_info *multiboot) {
	log_const("module test...");
	struct multiboot_mod mod = multiboot->mods[0];
	log_write(mod.start, mod.end - mod.start - 1);

	extern void _bss_end;
	long ptr = &_bss_end;
	log_const("_bss_end ");
	log_var_dont_use(ptr);
	extern void _data_end;
	ptr = &_data_end;
	log_const(" _data_end ");
	log_var_dont_use(ptr);
	log_const(" mod.start ");
	log_var_dont_use(mod.start);
	log_const(" mod.end ");
	log_var_dont_use(mod.end);
}

void kmain_early(struct multiboot_info *multiboot) {
	struct kmain_info info;

	// setup some basic stuff
	tty_clear();
	log_const("gdt...");
	gdt_init();
	log_const("idt...");
	idt_init();
	log_const("sysenter...");
	sysenter_setup();
	module_test(multiboot);
	
	{ // find the init module
		struct multiboot_mod *module = &multiboot->mods[0];
		if (multiboot->mods_count < 1) {
			log_const("can't find init! ");
			panic();
		}
		info.init.at   = module->start;
		info.init.size = module->end - module->start;
	}
	
	kmain(info);
}

#include <kernel/arch/generic.h>
#include <kernel/arch/i386/ata.h>
#include <kernel/arch/i386/boot.h>
#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/arch/i386/driver/serial.h>
#include <kernel/arch/i386/gdt.h>
#include <kernel/arch/i386/interrupts/idt.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/multiboot.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/vfs/root.h>

void tty_init(void); // TODO put this in a header file

static void find_init(struct multiboot_info *multiboot, struct kmain_info *info)
{
	struct multiboot_mod *module = &multiboot->mods[0];
	if (multiboot->mods_count < 1) {
		kprintf("can't find init! ");
		panic_invalid_state();
	}
	info->init.at   = module->start;
	info->init.size = module->end - module->start;

}

void kmain_early(struct multiboot_info *multiboot) {
	struct kmain_info info;

	tty_init();
	kprintf("gdt...");
	gdt_init();
	kprintf("idt...");
	idt_init();
	kprintf("irq...");
	irq_init();

	info.memtop = (void*) (multiboot->mem_upper * 1024);
	find_init(multiboot, &info);
	kprintf("mem...\n");
	mem_init(&info);

	kprintf("rootfs...");
	vfs_root_init();
	ps2_init();
	serial_init();

	kprintf("ata...");
	ata_init();

	kmain(info);
}

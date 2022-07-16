#include <kernel/arch/generic.h>
#include <kernel/arch/amd64/32/gdt.h>
#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/interrupts/idt.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/multiboot.h>
#include <kernel/arch/amd64/tty/tty.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>

static void find_init(struct multiboot_info *multiboot, struct kmain_info *info)
{
	struct multiboot_mod *module = (void*)multiboot->mods;
	kprintf("mods count 0x%x", multiboot->mods_count);
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

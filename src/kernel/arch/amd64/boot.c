#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/driver/video.h>
#include <kernel/arch/amd64/interrupts/idt.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/multiboot.h>
#include <kernel/arch/amd64/tty/tty.h>
#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>

static void find_init(struct multiboot_info *multiboot, struct kmain_info *info)
{
	struct multiboot_mod *module = (void*)(long)multiboot->mods;
	if (multiboot->mods_count != 1) {
		kprintf("unexpected amount of multiboot modules\n");
		panic_invalid_state();
	}
	info->init.at   = (void*)(long)module->start;
	info->init.size = module->end - module->start;
}

void kmain_early(struct multiboot_info *multiboot) {
	struct kmain_info info;
	struct fb_info vid;

	tty_init();
	kprintf("idt...");
	idt_init();
	kprintf("irq...");
	irq_init();

	info.memtop = (void*)(long)(multiboot->mem_upper * 1024);
	find_init(multiboot, &info);
	kprintf("mem...\n");
	mem_init(&info);

	kprintf("rootfs...");
	vfs_root_init();
	ps2_init();
	serial_init();

	kprintf("ata...");
	ata_init();

	vid.b      = (void*)multiboot->framebuffer_addr;
	vid.pitch  = multiboot->framebuffer_pitch;
	vid.width  = multiboot->framebuffer_width;
	vid.height = multiboot->framebuffer_height;
	vid.bpp    = multiboot->framebuffer_bpp;

	kprintf("framebuffer at 0x%x, %ux%u bpp %u\n", vid.b, vid.width, vid.height, vid.bpp);
	video_init(vid);


	kmain(info);
}

#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/arch/amd64/driver/pata.h>
#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/driver/video.h>
#include <kernel/arch/amd64/interrupts/idt.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/3rdparty/multiboot2.h>
#include <kernel/arch/amd64/tty/tty.h>
#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>

static void *mbi_tag(void *mbi, uint32_t type) {
	struct multiboot_tag *tag;
	for (tag = mbi + 8;
		tag->type != MULTIBOOT_TAG_TYPE_END;
		tag = ((void*)tag) + ((tag->size + 7) & ~7))
	{
		if (tag->type == type)
			return tag;
	}
	kprintf("bootloader didn't pass required tag type %u\n", type);
	panic_invalid_state();
}

void kmain_early(void *mbi) {
	struct kmain_info info;
	struct fb_info vid;

	tty_init();
	kprintf("idt...");
	idt_init();
	kprintf("irq...");
	irq_init();

	struct multiboot_tag_basic_meminfo *meminfo;
	meminfo = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
	info.memtop = (void*)(long)(meminfo->mem_upper * 1024);

	struct multiboot_tag_framebuffer *framebuf;
	framebuf = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
	vid.b      = (void*)framebuf->common.framebuffer_addr;
	vid.pitch  = framebuf->common.framebuffer_pitch;
	vid.width  = framebuf->common.framebuffer_width;
	vid.height = framebuf->common.framebuffer_height;
	vid.bpp    = framebuf->common.framebuffer_bpp;
	vid.size   = vid.pitch * vid.height;
	// TODO framebuffer doesn't need to be part of kmain_info
	info.fb.at   = vid.b;
	info.fb.size = vid.size;

	struct multiboot_tag_module *init;
	init = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_MODULE);
	info.init.at = (void*)(long)init->mod_start;
	info.init.size = init->mod_end - init->mod_start;

	kprintf("mem...\n");
	mem_init(&info);

	kprintf("rootfs...\n");
	vfs_root_init();
	ps2_init();
	serial_init();

	kprintf("ata...\n");
	pata_init();

	kprintf("kernel %8x -> %8x\n", 0, &_bss_end);
	kprintf("init   %8x -> %8x\n", info.init.at, info.init.at + info.init.size);
	kprintf("video  %8x -> %8x\n", vid.b, vid.b + vid.size);
	kprintf("limit  %8x\n", info.memtop);
	kprintf("framebuffer at 0x%x, %ux%u bpp %u\n", vid.b, vid.width, vid.height, vid.bpp);
	video_init(vid);

	kmain(info);
}

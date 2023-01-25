#include <kernel/arch/amd64/3rdparty/multiboot2.h>
#include <kernel/arch/amd64/boot.h>
#include <kernel/arch/amd64/driver/driver.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/interrupts.h>
#include <kernel/arch/amd64/pci.h>
#include <kernel/arch/amd64/tty/tty.h>
#include <kernel/arch/generic.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

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
	GfxInfo vid;
	struct {
		void *addr; size_t len;
	} init;

	tty_init();
	kprintf("idt...");
	idt_init();
	kprintf("irq...");
	irq_init();
	timer_init();

	{
		kprintf("mem...\n");
		struct multiboot_tag_basic_meminfo *meminfo;
		meminfo = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
		mem_init((void*)(long)(meminfo->mem_upper * 1024));
	}

	{
		struct multiboot_tag_framebuffer *framebuf;
		framebuf = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
		vid.b      = (void*)framebuf->common.framebuffer_addr;
		vid.pitch  = framebuf->common.framebuffer_pitch;
		vid.width  = framebuf->common.framebuffer_width;
		vid.height = framebuf->common.framebuffer_height;
		vid.bpp    = framebuf->common.framebuffer_bpp;
		vid.size   = vid.pitch * vid.height;
		mem_reserve(vid.b, vid.size);
		kprintf("framebuffer at 0x%x, %ux%u bpp %u\n", vid.b, vid.width, vid.height, vid.bpp);
	}

	{
		struct multiboot_tag_module *mod;
		mod = mbi_tag(mbi, MULTIBOOT_TAG_TYPE_MODULE);
		init.addr = (void*)(long)mod->mod_start;
		init.len = mod->mod_end - mod->mod_start;
		mem_reserve(init.addr, init.len);
	}

	kprintf("rootfs...\n");
	vfs_root_init();
	ps2_init();
	serial_init();
	video_init(vid);
	pata_init();

	kprintf("pci...\n");
	pci_init();

	kprintf("running init...\n");
	proc_seed(init.addr, init.len);
	proc_switch_any();
}

void shutdown(void) {
	kprintf("shutting off\n");
	mem_debugprint();
	cpu_shutdown();
}

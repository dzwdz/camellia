#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <stdint.h>

static void setup_paging() {
	struct pagedir *dir = pagedir_new();

	// map VGA
	pagedir_map(dir, 0xB8000, 0xB8000, true, true);

	// map the kernel
	for (size_t p = 0x100000; p < &_bss_end; p += PAGE_SIZE)
		pagedir_map(dir, p, p, false, true); // yes, .text is writeable too

	pagedir_use(dir);
}

void kmain(struct kmain_info info) {
	log_const("mem...");
	mem_init(&info);

	log_const("paging...");
	setup_paging();

	log_const("creating process...");

	void *init_addr = (void*)0x200000;
	memcpy(init_addr, info.init.at, info.init.size);

	struct process *proc = process_new(init_addr);
	log_const("switching...");
	process_switch(proc);
}

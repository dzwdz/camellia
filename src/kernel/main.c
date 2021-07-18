#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <stdint.h>

void r3_test();

void kmain(struct kmain_info info) {
	log_const("mem...");
	mem_init(&info);

	log_const("creating process...");

	void *init_addr = (void*)0x200000;
	memcpy(init_addr, info.init.at, info.init.size);

	struct process *proc = process_new(init_addr);
	log_const("switching...");
	process_switch(proc);
}

void r3_test() {
	log_const("ok");
	asm("cli");
	panic();
}

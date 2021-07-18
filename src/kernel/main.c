#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

void r3_test();

void kmain(struct kmain_info info) {
	log_write(info.init.at, info.init.size);
	log_const("mem...");
	mem_init(&info);

	log_const("creating process...");
	struct process *proc = process_new(r3_test);
	log_const("switching...");
	process_switch(proc);
}

void r3_test() {
	log_const("ok");
	asm("cli");
	panic();
}

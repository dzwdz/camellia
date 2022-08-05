#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <stdint.h>

void kmain(struct kmain_info info) {
	kprintf("loading init...\n");
	process_seed(&info);

	kprintf("switching...\n");
	process_switch_any();
}

void shutdown(void) {
	kprintf("shutting off\n");
	mem_debugprint();
	cpu_shutdown();
}

#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/proc.h>
#include <stdint.h>

struct process *process_current;

struct process *process_new() {
	struct process *proc = page_alloc(1); // TODO kmalloc
	proc->pages = pagedir_new();

	// map the stack to the last page in memory
	pagedir_map(proc->pages, (void*)~PAGE_MASK, page_alloc(1), true, true);
	proc->stack_top = proc->regs.esp = ~0xF;

	// map the kernel
	//   yup, .text is writeable too. the plan is to not map the kernel
	//   into user memory at all, but i'll implement that later. TODO
	for (size_t p = 0x100000; p < (size_t)&_bss_end; p += PAGE_SIZE)
		pagedir_map(proc->pages, (void*)p, (void*)p, false, true);

	// the kernel still has to load the executable code and set EIP
	return proc;
}

void process_switch(struct process *proc) {
	process_current = proc;
	pagedir_switch(proc->pages);
	sysexit(proc->regs);
}

#include <arch/generic.h>
#include <kernel/mem.h>
#include <kernel/proc.h>

struct process *process_current;

struct process *process_new(void *eip) {
	struct process *proc;
	proc = page_alloc(1); // TODO kmalloc
	proc->stack_top = proc->esp = page_alloc(1) + 1 * PAGE_SIZE;
	proc->eip = eip;

	return proc;
}

void process_switch(struct process *proc) {
	process_current = proc;
	sysexit(proc->eip, proc->esp);
}

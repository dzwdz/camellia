#include <kernel/mem.h>
#include <kernel/proc.h>
#include <platform/sysenter.h>

struct process *process_current;

struct process *process_new(void *eip) {
	struct process *proc;
	proc = malloc(sizeof(struct process));

	// should allocate an actual page. TODO
	proc->stack_top = proc->esp = malloc(4096);
	proc->eip = eip;

	return proc;
}

void process_switch(struct process *proc) {
	process_current = proc;
	sysexit(proc->eip, proc->esp);
}

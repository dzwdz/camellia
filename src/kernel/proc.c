#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <stdint.h>

struct process *process_first;
struct process *process_current;
uint32_t next_pid = 0;

struct process *process_seed() {
	struct process *proc = kmalloc(sizeof(struct process));
	proc->pages = pagedir_new();
	proc->state = PS_RUNNING;
	proc->sibling = NULL;
	proc->child   = NULL;
	proc->parent  = NULL;
	proc->mount   = NULL;
	proc->id      = next_pid++;

	process_first = proc;

	// map the stack to the last page in memory
	pagedir_map(proc->pages, (void*)~PAGE_MASK, page_alloc(1), true, true);
	proc->stack_top = (void*) (proc->regs.esp = ~0xF);

	// map the kernel
	//   yup, .text is writeable too. the plan is to not map the kernel
	//   into user memory at all, but i'll implement that later. TODO
	for (size_t p = 0x100000; p < (size_t)&_bss_end; p += PAGE_SIZE)
		pagedir_map(proc->pages, (void*)p, (void*)p, false, true);

	// the kernel still has to load the executable code and set EIP
	return proc;
}

struct process *process_fork(struct process *parent) {
	struct process *child = kmalloc(sizeof(struct process));
	memcpy(child, parent, sizeof(struct process));

	child->pages = pagedir_copy(parent->pages);
	child->sibling = parent->child;
	child->child   = NULL;
	child->parent  = parent;
	parent->child  = child;

	child->id      = next_pid++;

	return child;
}

void process_switch(struct process *proc) {
	process_current = proc;
	pagedir_switch(proc->pages);
	sysexit(proc->regs);
}

_Noreturn void process_switch_any() {
	struct process *found = process_find(PS_RUNNING);
	if (found)
		process_switch(found);

	tty_const(" no running processes left...");
	panic();
}

// TODO there's no check for going past the stack - VULN
struct process *_process_find_recursive(
		enum process_state target, struct process *iter) {
	struct process *in;
	while (iter) {
		if (iter->state == target)
			return iter;

		// DFS
		in = _process_find_recursive(target, iter->child);
		if (in)
			return in;

		iter = iter->sibling;
	}
	return NULL;
}

struct process *process_find(enum process_state target) {
	return _process_find_recursive(target, process_first);
}

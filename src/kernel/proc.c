#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/mount.h>
#include <stdint.h>

struct process *process_first;
struct process *process_current;
static uint32_t next_pid = 0;

struct process *process_seed(void) {
	struct process *proc = kmalloc(sizeof *proc);
	proc->pages = pagedir_new();
	proc->state = PS_RUNNING;
	proc->sibling = NULL;
	proc->child   = NULL;
	proc->parent  = NULL;
	proc->mount   = vfs_mount_seed();
	proc->id      = next_pid++;
	proc->handled_req = NULL;

	process_first = proc;

	for (int i = 0; i < HANDLE_MAX; i++)
		proc->handles[i].type = HANDLE_EMPTY;

	// map the stack to the last page in memory
	pagedir_map(proc->pages, (userptr_t)~PAGE_MASK, page_alloc(1), true, true);
	proc->regs.esp = (userptr_t) ~0xF;

	// map the kernel
	//   yup, .text is writeable too. the plan is to not map the kernel
	//   into user memory at all, but i'll implement that later. TODO
	for (size_t p = 0x100000; p < (size_t)&_bss_end; p += PAGE_SIZE)
		pagedir_map(proc->pages, (userptr_t)p, (void*)p, false, true);

	// the kernel still has to load the executable code and set EIP
	return proc;
}

struct process *process_fork(struct process *parent) {
	struct process *child = kmalloc(sizeof *child);
	memcpy(child, parent, sizeof *child);

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

_Noreturn void process_switch_any(void) {
	struct process *found = process_find(PS_RUNNING);
	if (found)
		process_switch(found);

	mem_debugprint();
	panic_unimplemented(); // TODO shutdown();
}

// TODO there's no check for going past the stack - VULN
static struct process *_process_find_recursive(
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

handle_t process_find_handle(struct process *proc) {
	handle_t handle;
	for (handle = 0; handle < HANDLE_MAX; handle++) {
		if (proc->handles[handle].type == HANDLE_EMPTY)
			break;
	}
	if (handle == HANDLE_MAX) handle = -1;
	return handle;
}

int process_try2collect(struct process *dead) {
	struct process *parent = dead->parent;
	int len, ret;
	bool res;

	assert(dead->state == PS_DEAD);

	switch (parent->state) {
		case PS_WAITS4CHILDDEATH:
			dead->state = PS_DEADER;
			parent->state = PS_RUNNING;

			ret = dead->death_msg;
			regs_savereturn(&parent->regs, ret);
			return ret;

		default:
			return -1; // this return value isn't used anywhere
			           // TODO enforce that, somehow? idk
	}
}

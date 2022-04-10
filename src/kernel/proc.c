#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>
#include <stdint.h>
#include <kernel/arch/i386/interrupts/irq.h> // TODO move irq stuff to arch/generic
#include <kernel/vfs/root.h> // TODO

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
	proc->controlled  = NULL;

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

	parent->handled_req = NULL; // TODO control this with a flag

	child->id      = next_pid++;

	return child;
}

void process_switch(struct process *proc) {
	assert(proc->state == PS_RUNNING);
	process_current = proc;
	pagedir_switch(proc->pages);
	sysexit(proc->regs);
}

_Noreturn void process_switch_any(void) {
	struct process *found = process_find(PS_RUNNING);
	if (found) process_switch(found);
	process_idle();
}

_Noreturn void process_idle(void) {
	struct process *procs[16];
	size_t len = process_find_multiple(PS_WAITS4IRQ, procs, 16);

	if (len == 0) {
		mem_debugprint();
		cpu_shutdown();
	}

	irq_interrupt_flag(true);
	for (;;) {
		asm("hlt" ::: "memory"); // TODO move to irq.c
		for (size_t i = 0; i < len; i++) {
			if (procs[i]->waits4irq.ready()) {
				irq_interrupt_flag(false);
				vfs_root_handler(&procs[i]->waits4irq.req); // TODO this should be a function pointer too
				process_switch_any();
			}
		}
	}
}

// TODO there's no check for going past the stack - VULN
static size_t _process_find_recursive(
		enum process_state target, struct process *iter,
		struct process **buf, size_t pos, size_t max)
{
	while (pos < max && iter) {
		if (iter->state == target)
			buf[pos++] = iter;

		// DFS
		pos = _process_find_recursive(target, iter->child, buf, pos, max);

		iter = iter->sibling;
	}
	return pos;
}

struct process *process_find(enum process_state target) {
	struct process *result = NULL;
	process_find_multiple(target, &result, 1);
	return result;
}

size_t process_find_multiple(enum process_state target, struct process **buf, size_t max) {
	return _process_find_recursive(target, process_first, buf, 0, max);
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

void process_kill(struct process *proc, int ret) {
	proc->state = PS_DEAD;
	proc->death_msg = ret;
	process_try2collect(proc);
}

int process_try2collect(struct process *dead) {
	struct process *parent = dead->parent;
	int ret;

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

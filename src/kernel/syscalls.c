#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/syscalls.h>
#include <stdint.h>

_Noreturn void _syscall_exit(const char *msg, size_t len) {
	process_current->state = PS_DEAD;

	// check if our parent is awaiting our death
	if (process_current->parent->state == PS_WAITS4CHILDDEATH) {
		// how cruel
		process_current->state = PS_DEADER;
		process_current->parent->state = PS_RUNNING;
		process_switch(process_current->parent);
	}

	process_current = process_find(PS_RUNNING);
	if (process_current)
		process_switch(process_current);

	tty_const("last process returned. ");
	panic();
}

int _syscall_await(char *buf, int *len) {
	// find any already dead children
	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling) {
		if (iter->state == PS_DEAD) {
			iter->state = PS_DEADER;
			// TODO copy return message
			return 0;
		}
	}

	// no dead children yet
	// TODO check if the process even has children

	process_current->state = PS_WAITS4CHILDDEATH;
	process_current = process_find(PS_RUNNING);
	if (process_current)
		process_switch(process_current);

	tty_const("this error will be fixed in the next commit."); // TODO
	panic();
}

int _syscall_fork() {
	struct process *child = process_fork(process_current);
	regs_savereturn(&child->regs, 0);
	return 1;
}

int _syscall_debuglog(const char *msg, size_t len) {
	struct virt_iter iter;
	size_t written;

	virt_iter_new(&iter, msg, len, process_current->pages, true, false);
	while (virt_iter_next(&iter)) {
		tty_write(iter.frag, iter.frag_len);
		written += iter.frag_len;
	}
	return written;
}

int syscall_handler(int num, int a, int b, int c) {
	switch (num) {
		case _SYSCALL_EXIT:
			_syscall_exit((void*)a, b);
		case _SYSCALL_AWAIT:
			return _syscall_await((void*)a, (void*)b);
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_DEBUGLOG:
			return _syscall_debuglog((void*)a, b);
		default:
			tty_const("unknown syscall ");
			panic();
	}
}

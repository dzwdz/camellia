#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/syscalls.h>
#include <stdint.h>

_Noreturn void _syscall_exit(const char *msg, size_t len) {
	process_current->state = PS_DEAD;

	process_current = process_find(PS_RUNNING);
	if (process_current)
		process_switch(process_current);

	tty_const("last process returned. ");
	panic();
}

int _syscall_fork() {
	struct process *orig = process_current;
	process_current = process_fork(orig);
	process_switch(process_current);
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
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_DEBUGLOG:
			return _syscall_debuglog((void*)a, b);
		default:
			tty_const("unknown syscall ");
			panic();
	}
}

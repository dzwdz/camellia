#include <kernel/arch/generic.h>
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
	process_current = process_clone(orig);
	process_switch(process_current);
}

int _syscall_debuglog(const char *msg, size_t len) {
	struct pagedir *pages = process_current->pages;
	size_t remaining = len;

	while (remaining > 0) {
		/* note: While i'm pretty sure that this should always work, this
		 * was only tested in cases where the pages were consecutive both in
		 * virtual and physical memory, which might not always be the case.
		 * TODO test this */

		void *phys = pagedir_virt2phys(pages, msg, true, false);
		size_t partial = remaining;

		// don't read past the page
		if (((uintptr_t)msg & PAGE_MASK) + remaining > PAGE_SIZE)
			partial = PAGE_SIZE - ((uintptr_t)msg & PAGE_MASK);

		tty_write(phys, partial);

		remaining -= partial;
		msg       += partial;
	}

	return len;
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

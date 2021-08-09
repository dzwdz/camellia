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
	void *phys = pagedir_virt2phys(pages, msg, true, false);

	// page overrun check
	if (((uintptr_t)msg & PAGE_MASK) + len > PAGE_SIZE)
		len = PAGE_SIZE - ((uintptr_t)msg & PAGE_MASK);
	if (((uintptr_t)msg & PAGE_MASK) + len > PAGE_SIZE)
		panic(); // just in case I made an off by 1 error

	tty_write(phys, len);
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

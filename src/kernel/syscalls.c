#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/syscalls.h>
#include <kernel/vfs/path.h>
#include <stdint.h>

_Noreturn static void await_finish(struct process *dead, struct process *listener) {
	size_t len;
	bool res;

	assert(dead->state == PS_DEAD);
	assert(listener->state == PS_WAITS4CHILDDEATH);
	dead->state = PS_DEADER;
	listener->state = PS_RUNNING;

	len = listener->saved_len < dead->saved_len
	    ? listener->saved_len : dead->saved_len;
	res = virt_user_cpy(
			listener->pages, listener->saved_addr,
			dead->pages, dead->saved_addr, len);
	regs_savereturn(&listener->regs, res ? len : -1);
	process_switch(listener);
}


_Noreturn void _syscall_exit(const char *msg, size_t len) {
	process_current->state = PS_DEAD;
	process_current->saved_addr = (void*)msg; // discard const
	process_current->saved_len  = len;

	if (process_current->parent->state == PS_WAITS4CHILDDEATH)
		await_finish(process_current, process_current->parent);

	process_current = process_find(PS_RUNNING);
	if (process_current)
		process_switch(process_current);

	process_switch_any();
}

int _syscall_await(char *buf, int len) {
	process_current->state = PS_WAITS4CHILDDEATH;
	process_current->saved_addr = buf;
	process_current->saved_len  = len;

	// find any already dead children
	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling)
	{
		if (iter->state == PS_DEAD)
			await_finish(iter, process_current); // doesn't return
	}

	// no dead children yet
	// TODO check if the process even has children
	
	process_switch_any();
}

int _syscall_fork() {
	struct process *child = process_fork(process_current);
	regs_savereturn(&child->regs, 0);
	return 1;
}

fd_t _syscall_fs_open(const char *path, size_t len) {
	struct virt_iter iter;
	static char buffer[PATH_MAX]; // holds the simplified path
	size_t pos = 0;

	if (len > PATH_MAX) return -1;

	// copy the path to buffer
	virt_iter_new(&iter, (void*)path, len, process_current->pages, true, false);
	while (virt_iter_next(&iter)) {
		memcpy(buffer + pos, iter.frag, iter.frag_len);
		pos += iter.frag_len;
	}
	if (iter.error) return -1;

	len = path_simplify(buffer, buffer, len);
	if (len < 0) return -1;

	tty_write(buffer, len);
	return -1;
}

int _syscall_debuglog(const char *msg, size_t len) {
	struct virt_iter iter;
	size_t written = 0;

	virt_iter_new(&iter, (void*)msg, len, process_current->pages, true, false);
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
			return _syscall_await((void*)a, b);
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_FS_OPEN:
			return _syscall_fs_open((void*)a, b);
		case _SYSCALL_DEBUGLOG:
			return _syscall_debuglog((void*)a, b);
		default:
			tty_const("unknown syscall ");
			panic();
	}
}

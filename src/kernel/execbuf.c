#include <camellia/execbuf.h>
#include <camellia/syscalls.h>
#include <kernel/execbuf.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <shared/mem.h>

_Noreturn static void halt(Proc *proc) {
	kfree(proc->execbuf.buf);
	proc->execbuf.buf = NULL;
	proc_switch_any();
}

static void try_fetch(Proc *proc, uint64_t *buf, size_t amt) {
	size_t bytes = amt * sizeof(uint64_t);
	if (proc->execbuf.pos + bytes > proc->execbuf.len)
		halt(proc);
	memcpy(buf, proc->execbuf.buf + proc->execbuf.pos, bytes);
	proc->execbuf.pos += bytes;
}

_Noreturn void execbuf_run(Proc *proc) {
	uint64_t buf[6];

	assert(proc == proc_cur); // idiotic, but needed because of _syscall.
	assert(proc->state == PS_RUNNING);
	assert(proc->execbuf.buf);

	try_fetch(proc, buf, 1);
	switch (buf[0]) {
		case EXECBUF_SYSCALL:
			try_fetch(proc, buf, 6);
			_syscall(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
			proc_switch_any();

		case EXECBUF_JMP:
			try_fetch(proc, buf, 1);
			proc->regs.rcx = buf[0];
			execbuf_run(proc);

		default:
			halt(proc);
	}
}

#include <kernel/arch/generic.h>
#include <kernel/arch/amd64/sysenter.h>
#include <kernel/proc.h>
#include <shared/syscalls.h>

struct registers _sysexit_regs;

void sysexit(struct registers regs) {
	_sysexit_regs = regs;
	kprintf("ring3...\n");
	_sysexit_real();
	__builtin_unreachable();
}

_Noreturn void sysenter_stage2(void) {
	struct registers *regs = &process_current->regs;
	*regs = _sysexit_regs;

	kprintf("ring0!\n");

	_syscall(regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8);
	process_switch_any();
}

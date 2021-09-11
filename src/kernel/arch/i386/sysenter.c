#include <kernel/arch/generic.h>
#include <kernel/proc.h>

struct registers _sysexit_regs; // a hack

extern void _sysexit_real(void);

void sysexit(struct registers regs) {
	_sysexit_regs = regs;
	_sysexit_regs.ecx = (uintptr_t) regs.esp;
	_sysexit_regs.edx = (uintptr_t) regs.eip;
	_sysexit_real();
	__builtin_unreachable();
}

_Noreturn void sysenter_stage2(void) {
	uint64_t val;
	struct registers *regs = &process_current->regs;

	*regs = _sysexit_regs; // save the registers
	regs->esp = (userptr_t) regs->ecx; // fix them up
	regs->eip = (userptr_t) regs->edx;

	val = syscall_handler(regs->eax, regs->ebx,
	                      regs->esi, regs->edi);
	regs_savereturn(&process_current->regs, val);

	process_switch(process_current); // TODO process_resume
}

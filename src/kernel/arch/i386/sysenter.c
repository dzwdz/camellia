#include <kernel/arch/generic.h>
#include <kernel/proc.h>

struct registers _sysexit_regs; // a hack

extern void _sysexit_real();

void sysexit(struct registers regs) {
	_sysexit_regs = (struct registers) {
		.eax = regs.eax,
		.ebx = regs.ebx,
		.ebp = regs.ebp,
		.esi = regs.esi,
		.edi = regs.edi,

		// sysexit args
		.ecx = regs.esp,
		.edx = regs.eip,

		// ESP doesn't matter
	};
	_sysexit_real();
}

_Noreturn void sysenter_stage2() {
	uint64_t val;
	struct registers *regs = &process_current->regs;

	*regs = _sysexit_regs; // save the registers
	regs->esp = regs->ecx; // fix them up
	regs->eip = regs->edx;

	val = syscall_handler(regs->eax, regs->ebx,
	                      regs->esi, regs->edi);
	regs_savereturn(&process_current->regs, val);

	process_switch(process_current); // TODO process_resume
}

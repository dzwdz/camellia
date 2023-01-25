#include <camellia/syscalls.h>
#include <kernel/arch/amd64/sysenter.h>
#include <kernel/arch/generic.h>
#include <kernel/proc.h>

CpuRegs _sysexit_regs;

_Noreturn void sysexit(CpuRegs regs) {
	_sysexit_regs = regs;
	_sysexit_real();
}

_Noreturn void sysenter_stage2(void) {
	CpuRegs *regs = &proc_cur->regs;
	*regs = _sysexit_regs;
	_syscall(regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9);
	proc_switch_any();
}

#include <kernel/arch/generic.h>
#include <kernel/proc.h>

void sysenter_stage2(int edi, int esi, void *ebp, void *esp,
                     int ebx, int edx, int   ecx, int   eax)
{
	uint64_t val;
	process_current->regs = (struct registers) {
		// EAX and EDX will get overriden with the return value later on

		.eax = eax,
		.ecx = ecx,
		.edx = edx,
		.ebx = ebx,
		.esi = esi,
		.edi = edi,

		.esp = (void*) ecx, // not a typo, part of my calling convention
		.eip = (void*) edx, // ^
		.ebp = ebp,
	};

	val = syscall_handler(eax, ebx, esi, edi);
	regs_savereturn(&process_current->regs, val);

	process_switch(process_current); // TODO process_resume
}

#pragma once
#include <kernel/arch/generic.h>

struct process {
	void *stack_top;
	struct pagedir *pages;
	struct registers regs;
};

extern struct process *process_current;

struct process *process_new();
_Noreturn void process_switch(struct process *proc);

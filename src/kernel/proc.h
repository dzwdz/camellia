#pragma once
#include <kernel/arch/generic.h>

struct process {
	void *stack_top;
	void *esp;
	void *eip;

	struct pagedir *pages;
};

extern struct process *process_current;

struct process *process_new();
void process_switch(struct process *proc);

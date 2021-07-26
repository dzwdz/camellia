#pragma once
#include <kernel/arch/generic.h>

enum process_state {
	PS_RUNNING,
	PS_DEAD,
};

struct process {
	void *stack_top;
	struct pagedir *pages;
	struct registers regs;
	enum process_state state;

	struct process *next;
};

extern struct process *process_first;
extern struct process *process_current;

struct process *process_new();
struct process *process_clone(struct process *orig);
_Noreturn void process_switch(struct process *proc);

struct process *process_find(enum process_state);

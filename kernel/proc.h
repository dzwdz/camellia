#pragma once

struct process {
	void *stack_top;
	void *esp;

	void *eip;
};

extern struct process *process_current;

struct process *process_new(void *eip);
void process_switch(struct process *proc);

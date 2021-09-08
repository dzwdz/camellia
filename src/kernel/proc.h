#pragma once
#include <kernel/arch/generic.h>
#include <kernel/handle.h>
#include <kernel/vfs/mount.h>

enum process_state {
	PS_RUNNING,
	PS_DEAD,    // return message wasn't collected
	PS_DEADER,  // return message was collected
	PS_WAITS4CHILDDEATH,
	PS_WAITS4FS,
	PS_WAITS4REQUEST,
};

struct process {
	user_ptr stack_top;
	struct pagedir *pages;
	struct registers regs;
	enum process_state state;

	struct process *sibling;
	struct process *child;
	struct process *parent;

	uint32_t id; // only for debugging, don't expose to userland

	// meaning changes depending on the state
	// PS_DEAD - death message
	// PS_WAITS4CHILDDEATH - buffer for said message
	user_ptr saved_addr;
	size_t saved_len;

	struct vfs_mount *mount;

	struct handle handles[HANDLE_MAX];
};

extern struct process *process_first;
extern struct process *process_current;

// creates the root process
struct process *process_seed(void);
struct process *process_fork(struct process *parent);
_Noreturn void process_switch(struct process *proc);
_Noreturn void process_switch_any(void); // switches to any running process

struct process *process_find(enum process_state);
handle_t process_find_handle(struct process *proc); // finds the first free handle

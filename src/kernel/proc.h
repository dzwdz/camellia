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

	// saved value, meaning depends on .state
	union {
		struct { // PS_DEAD, PS_WAITS4CHILDDEATH
			user_ptr buf;
			size_t len;
		} death_msg;
		struct vfs_op_request *pending_req; // PS_WAITS4FS
	};

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

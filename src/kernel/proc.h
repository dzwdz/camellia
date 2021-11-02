#pragma once
#include <kernel/arch/generic.h>
#include <kernel/handle.h>
#include <kernel/vfs/mount.h>
#include <shared/syscalls.h>

enum process_state {
	PS_RUNNING,
	PS_DEAD,    // return message wasn't collected
	PS_DEADER,  // return message was collected
	PS_WAITS4CHILDDEATH,
	PS_WAITS4FS,
	PS_WAITS4REQUEST,
};

struct process {
	struct pagedir *pages;
	struct registers regs;
	enum process_state state;

	struct process *sibling;
	struct process *child;
	struct process *parent;

	uint32_t id; // only for debugging, don't expose to userland

	// saved value, meaning depends on .state
	union {
		int death_msg; // PS_DEAD
		struct vfs_request pending_req; // PS_WAITS4FS
		struct {
			char __user *buf;
			int  max_len;
			struct fs_wait_response __user *res;
		} awaited_req; // PS_WAITS4REQUEST
	};
	struct vfs_request *handled_req;

	/* vfs_backend controlled (not exclusively) by this process */
	struct vfs_backend *controlled;

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

void process_kill(struct process *proc, int ret);

/** Tries to transistion from PS_DEAD to PS_DEADER.
 * @return a nonnegative length of the quit message if successful, a negative val otherwise*/
int process_try2collect(struct process *dead);

#pragma once
#include <kernel/arch/generic.h>
#include <kernel/handle.h>
#include <kernel/main.h>
#include <kernel/vfs/mount.h>
#include <shared/syscalls.h>
#include <stdbool.h>

#define HANDLE_MAX 16

enum process_state {
	PS_RUNNING,
	PS_DEAD, // return message not collected
	PS_WAITS4CHILDDEATH,
	PS_WAITS4FS,
	PS_WAITS4REQUEST,
	PS_WAITS4PIPE,

	PS_LAST,
};

struct process {
	struct pagedir *pages;
	struct registers regs;
	enum process_state state;

	bool noreap;

	struct process *sibling;
	struct process *child;
	struct process *parent;

	uint32_t id; // only for debugging, don't expose to userland

	// saved value, meaning depends on .state
	union {
		int death_msg; // PS_DEAD
		struct {
			struct vfs_request *req;
		} waits4fs; // PS_WAITS4FS
		struct {
			char __user *buf;
			size_t max_len;
			struct fs_wait_response __user *res;
		} awaited_req; // PS_WAITS4REQUEST
		struct {
			struct handle *pipe;
			char __user *buf;
			size_t len;
			struct process *next;
		} waits4pipe;
	};
	struct vfs_request *handled_req;

	/* vfs_backend controlled (not exclusively) by this process */
	struct vfs_backend *controlled;

	struct vfs_mount *mount;

	struct handle *handles[HANDLE_MAX];
};

extern struct process *process_first;
extern struct process *process_current;

/** Creates the root process. */
struct process *process_seed(struct kmain_info *info);
struct process *process_fork(struct process *parent, int flags);

void process_kill(struct process *proc, int ret);
/** Tries to free a process / collect its return value. */
int process_try2collect(struct process *dead);
void process_free(struct process *);
/** Removes a process from the process tree. */
void process_forget(struct process *);

/** Switches execution to any running process. */
_Noreturn void process_switch_any(void);

/** Used for iterating over all processes */
struct process *process_next(struct process *);

struct process *process_find(enum process_state);

handle_t process_find_free_handle(struct process *proc, handle_t start_at);
struct handle *process_handle_get(struct process *, handle_t);

void process_transition(struct process *, enum process_state);

#pragma once
#include <kernel/arch/generic.h>
#include <kernel/handle.h>
#include <kernel/types.h>
#include <stdbool.h>

#define HANDLE_MAX 16

/* legal transitions described by proc_setstate */
enum proc_state {
	PS_RUNNING,
	PS_DYING, /* during proc_kill - mostly treated as alive */
	PS_TOREAP, /* return message not collected */
	PS_TOMBSTONE, /* fully dead, supports alive children */
	/* not in the process tree, waits for free.
	 * *sibling holds a linked list of all the FORGOTTEN processes */
	PS_FORGOTTEN,
	PS_FREED, /* only meant to detect double frees */

	PS_WAITS4CHILDDEATH,
	PS_WAITS4FS,
	PS_WAITS4REQUEST,
	PS_WAITS4PIPE,
	PS_WAITS4TIMER,

	PS_LAST,
};

#define proc_alive(p) (p \
		&& p->state != PS_TOREAP \
		&& p->state != PS_TOMBSTONE \
		&& p->state != PS_FORGOTTEN \
		&& p->state != PS_FREED \
		)

struct Proc {
	Pagedir *pages;
	/* if NULL, refcount == 1. kmalloc'd */
	uint64_t *pages_refcount;

	CpuRegs regs;
	Proc *sibling, *child, *parent;

	enum proc_state state;
	union { /* saved value, meaning depends on .state */
		int death_msg; // PS_DEAD
		struct {
			char __user *buf;
			size_t max_len;
			struct ufs_request __user *res;
		} awaited_req; // PS_WAITS4REQUEST
		struct {
			Handle *pipe;
			char __user *buf;
			size_t len;
			Proc *next;
		} waits4pipe;
		struct {
			/* managed by timer_schedule */
			uint64_t goal;
			Proc *next;
		} waits4timer;
	};

	VfsMount *mount;
	Handle **_handles; /* points to Handle *[HANDLE_MAX] */
	uint64_t *handles_refcount; /* works just like pages_refcount */
	struct {
		Handle *procfs;
	} specialh;

	uint32_t globalid; /* only for internal use, don't expose to userland */
	uint32_t refcount; /* non-owning. should always be 0 on kill */
	bool noreap;

	/* localid is unique in a process namespace.
	 * if pns == self: the process owns a namespace
	 *                 the lid it sees is 1
	 *                 the lid its parent sees is localid
	 * otheriwse: nextlid is unused */
	Proc *pns;
	uint32_t localid;
	uint32_t nextlid;

	/* allocated once, the requests from WAITS4FS get stored here */
	VfsReq *reqslot;

	/* vfs_backend controlled (not exclusively) by this process */
	VfsBackend *controlled;

	/* interrupt handler */
	void __user *intr_fn;

	struct {
		void *buf;
		size_t len;
		size_t pos;
	} execbuf;
};

extern Proc *proc_cur;

/** Creates the root process. */
Proc *proc_seed(void *data, size_t datalen);
Proc *proc_fork(Proc *parent, int flags);

bool proc_ns_contains(Proc *ns, Proc *proc);
uint32_t proc_ns_id(Proc *ns, Proc *proc);
Proc *proc_ns_byid(Proc *ns, uint32_t id);
/** Like proc_next, but stays in *ns */
Proc *proc_ns_next(Proc *ns, Proc *p);
void proc_ns_create(Proc *proc);

void proc_kill(Proc *proc, int ret);
/** Kills all descendants. */
void proc_filicide(Proc *proc, int ret);
/** Tries to reap a dead process / free a tombstone. */
void proc_tryreap(Proc *dead);

void proc_intr(Proc *proc);

/** Switches execution to any running process. */
_Noreturn void proc_switch_any(void);

/** Used for iterating over all processes */
Proc *proc_next(Proc *p, Proc *root);

hid_t proc_find_free_handle(Proc *proc, hid_t start_at);
Handle *proc_handle_get(Proc *, hid_t);
hid_t proc_handle_init(Proc *, enum handle_type, Handle **);
hid_t proc_handle_dup(Proc *p, hid_t from, hid_t to);
static inline void proc_handle_close(Proc *p, hid_t hid) {
	// TODO test
	proc_handle_dup(p, -1, hid);
}

/* Gets a handle and removes the process' reference to it, without decreasing the refcount.
 * Meant to be used together with proc_handle_put. */
Handle *proc_hid_take(Proc *, hid_t);
/* Put a handle in a process, taking the ownership away from the caller.
 * Doesn't increase the refcount on success, decreases it on failure. */
hid_t proc_handle_put(Proc *, Handle *);

void proc_setstate(Proc *, enum proc_state);

size_t pcpy_to(Proc *p, __user void *dst, const void *src, size_t len);
size_t pcpy_from(Proc *p, void *dst, const __user void *src, size_t len);
size_t pcpy_bi(
	Proc *dstp, __user void *dst,
	Proc *srcp, const __user void *src,
	size_t len
);

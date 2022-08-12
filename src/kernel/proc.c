#include <kernel/arch/generic.h>
#include <kernel/execbuf.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>
#include <stdint.h>

static struct process *process_first = NULL;
struct process *process_current;

static uint32_t next_pid = 0;


/** Removes a process from the process tree. */
static void process_forget(struct process *p);
static _Noreturn void process_switch(struct process *proc);


struct process *process_seed(void *data, size_t datalen) {
	assert(!process_first);
	process_first = kmalloc(sizeof *process_first);
	memset(process_first, 0, sizeof *process_first);
	process_first->state = PS_RUNNING;
	process_first->pages = pagedir_new();
	process_first->mount = vfs_mount_seed();
	process_first->id    = next_pid++;

	// map the stack to the last page in memory
	// TODO move to user bootstrap
	pagedir_map(process_first->pages, (userptr_t)~PAGE_MASK, page_zalloc(1), true, true);
	process_first->regs.rsp = (userptr_t) ~0xF;

	// map .shared
	extern char _shared_len;
	for (size_t p = 0; p < (size_t)&_shared_len; p += PAGE_SIZE)
		pagedir_map(process_first->pages, (userptr_t)p, (void*)p, false, true);

	// map the init module as rw
	void __user *init_base = (userptr_t)0x200000;
	for (uintptr_t off = 0; off < datalen; off += PAGE_SIZE)
		pagedir_map(process_first->pages, init_base + off, data + off, true, true);
	process_first->regs.rcx = (uintptr_t)init_base; // SYSRET jumps to %rcx

	return process_first;
}

struct process *process_fork(struct process *parent, int flags) {
	struct process *child = kmalloc(sizeof *child);
	memset(child, 0, sizeof *child);

	child->pages = pagedir_copy(parent->pages);
	child->regs  = parent->regs;
	child->state = parent->state;
	assert(child->state == PS_RUNNING); // not copying the state union

	child->noreap  = (flags & FORK_NOREAP) > 0;

	child->sibling = parent->child;
	child->child   = NULL;
	child->parent  = parent;
	parent->child  = child;

	child->id = next_pid++;

	// TODO control this with a flag
	child->handled_req  = parent->handled_req;
	parent->handled_req = NULL;

	if ((flags & FORK_NEWFS) == 0 && parent->controlled) {
		child->controlled = parent->controlled;
		child->controlled->potential_handlers++;
		child->controlled->refcount++;
	}

	child->mount = parent->mount;
	assert(child->mount);
	child->mount->refs++;

	for (handle_t h = 0; h < HANDLE_MAX; h++) {
		child->handles[h] = parent->handles[h];
		if (child->handles[h])
			child->handles[h]->refcount++;
	}

	return child;
}

void process_kill(struct process *p, int ret) {
	if (p->state != PS_DEAD) {
		if (p->handled_req) {
			vfsreq_finish_short(p->handled_req, -1);
			p->handled_req = NULL;
		}

		if (p->controlled) {
			// TODO vfs_backend_user_handlerdown
			assert(p->controlled->potential_handlers > 0);
			p->controlled->potential_handlers--;
			if (p->controlled->potential_handlers == 0) {
				// orphaned
				struct vfs_request *q = p->controlled->queue;
				while (q) {
					struct vfs_request *q2 = q->queue_next;
					vfsreq_finish_short(q, -1);
					q = q2;
				}
				p->controlled->queue = NULL;
			}
			if (p->controlled->user.handler == p) {
				assert(p->state == PS_WAITS4REQUEST);
				p->controlled->user.handler = NULL;
			}

			vfs_backend_refdown(p->controlled);
			p->controlled = NULL;
		}

		if (p->state == PS_WAITS4FS) {
			assert(p->reqslot);
			p->reqslot->caller = NULL; /* transfer ownership */
			p->reqslot = NULL;
		} else if (p->reqslot) {
			kfree(p->reqslot);
		}

		if (p->state == PS_WAITS4PIPE) {
			struct process **iter = &p->waits4pipe.pipe->pipe.queued;
			while (*iter && *iter != p) {
				assert((*iter)->state == PS_WAITS4PIPE);
				iter = &(*iter)->waits4pipe.next;
			}
			assert(iter && *iter == p);
			*iter = p->waits4pipe.next;
		}

		if (p->state == PS_WAITS4TIMER)
			timer_deschedule(p);

		for (handle_t h = 0; h < HANDLE_MAX; h++)
			handle_close(p->handles[h]);

		vfs_mount_remref(p->mount);
		p->mount = NULL;

		process_transition(p, PS_DEAD);
		p->death_msg = ret;

		if (p->execbuf.buf) {
			kfree(p->execbuf.buf);
			p->execbuf.buf = NULL;
		}

		pagedir_free(p->pages);

		// TODO VULN unbounded recursion
		struct process *c2;
		for (struct process *c = p->child; c; c = c2) {
			c2 = c->sibling;
			process_kill(c, -1);
		}
	}

	assert(!p->child);
	process_try2collect(p);

	if (p == process_first) shutdown();
}

void process_try2collect(struct process *dead) {
	assert(dead && dead->state == PS_DEAD);
	assert(!dead->child);

	struct process *parent = dead->parent;
	if (!dead->noreap && parent && parent->state != PS_DEAD) { /* reapable? */
		if (parent->state != PS_WAITS4CHILDDEATH)
			return; /* don't reap yet */
		regs_savereturn(&parent->regs, dead->death_msg);
		process_transition(parent, PS_RUNNING);
	}

	if (dead != process_first) {
		process_forget(dead);
		kfree(dead);
	}
}

/** Removes a process from the process tree. */
static void process_forget(struct process *p) {
	assert(p->parent);

	if (p->parent->child == p) {
		p->parent->child = p->sibling;
	} else {
		// this would be simpler if siblings were a doubly linked list
		struct process *prev = p->parent->child;
		while (prev->sibling != p) {
			prev = prev->sibling;
			assert(prev);
		}
		prev->sibling = p->sibling;
	}
}

static _Noreturn void process_switch(struct process *proc) {
	assert(proc->state == PS_RUNNING);
	process_current = proc;
	pagedir_switch(proc->pages);
	if (proc->execbuf.buf)
		execbuf_run(proc);
	else
		sysexit(proc->regs);
}

_Noreturn void process_switch_any(void) {
	for (;;) {
		if (process_current && process_current->state == PS_RUNNING)
			process_switch(process_current);

		for (struct process *p = process_first; p; p = process_next(p)) {
			if (p->state == PS_RUNNING)
				process_switch(p);
		}

		cpu_pause();
	}
}

struct process *process_next(struct process *p) {
	/* depth-first search, the order is:
	 *         1
	 *        / \
	 *       2   5
	 *      /|   |\
	 *     3 4   6 7
	 */
	if (!p) return NULL;
	if (p->child) return p->child;
	if (p->sibling) return p->sibling;

	/* looking at the diagram above - we're at 4, want to find 5 */
	while (!p->sibling) {
		p = p->parent;
		if (!p) return NULL;
	}
	return p->sibling;
}

handle_t process_find_free_handle(struct process *proc, handle_t start_at) {
	for (handle_t hid = start_at; hid < HANDLE_MAX; hid++) {
		if (proc->handles[hid] == NULL)
			return hid;
	}
	return -1;
}

struct handle *process_handle_get(struct process *p, handle_t id) {
	if (id < 0 || id >= HANDLE_MAX) return NULL;
	return p->handles[id];
}

void process_transition(struct process *p, enum process_state state) {
	assert(p->state != PS_DEAD);
	if (state != PS_RUNNING && state != PS_DEAD)
		assert(p->state == PS_RUNNING);
	p->state = state;
}

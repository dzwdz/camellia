#include <kernel/arch/generic.h>
#include <kernel/main.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>
#include <shared/syscalls.h>
#include <stdint.h>

struct process *process_first;
struct process *process_current;

static uint32_t next_pid = 0;

struct process *process_seed(struct kmain_info *info) {
	process_first = kmalloc(sizeof *process_first);
	memset(process_first, 0, sizeof *process_first);
	process_first->state = PS_RUNNING;
	process_first->pages = pagedir_new();
	process_first->mount = vfs_mount_seed();
	process_first->id    = next_pid++;

	// map the stack to the last page in memory
	pagedir_map(process_first->pages, (userptr_t)~PAGE_MASK, page_alloc(1), true, true);
	process_first->regs.esp = (userptr_t) ~0xF;

	// map .shared
	extern char _shared_len;
	for (size_t p = 0; p < (size_t)&_shared_len; p += PAGE_SIZE)
		pagedir_map(process_first->pages, (userptr_t)p, (void*)p, false, true);

	// map the init module as rw
	void __user *init_base = (userptr_t)0x200000;
	for (uintptr_t off = 0; off < info->init.size; off += PAGE_SIZE)
		pagedir_map(process_first->pages, init_base + off, info->init.at + off,
		            true, true);
	process_first->regs.eip = init_base;

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
		// TODO would it be better to change the default to not sharing the controlled fs?
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

		if (p->state == PS_WAITS4FS)
			p->waits4fs.req->caller = NULL;

		if (p->state == PS_WAITS4PIPE) {
			struct process **iter = &p->waits4pipe.pipe->pipe.queued;
			while (*iter && *iter != p) {
				assert((*iter)->state == PS_WAITS4PIPE);
				iter = &(*iter)->waits4pipe.next;
			}
			assert(iter && *iter == p);
			*iter = p->waits4pipe.next;
		}

		for (handle_t h = 0; h < HANDLE_MAX; h++)
			handle_close(p->handles[h]);

		vfs_mount_remref(p->mount);
		p->mount = NULL;

		process_transition(p, PS_DEAD);
		p->death_msg = ret;

		if (p->parent)
			pagedir_free(p->pages); // TODO put init's pages in the allocator

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

int process_try2collect(struct process *dead) {
	struct process *parent = dead->parent;
	int ret = -1;

	assert(dead && dead->state == PS_DEAD);

	if (!dead->noreap && parent && parent->state != PS_DEAD) { // might be reaped
		if (parent->state != PS_WAITS4CHILDDEATH) return -1;

		ret = dead->death_msg;
		regs_savereturn(&parent->regs, ret);
		process_transition(parent, PS_RUNNING);
	}

	process_free(dead);
	return ret;
}

void process_free(struct process *p) {
	assert(p->state == PS_DEAD);
	assert(!p->child);

	if (!p->parent) return;
	process_forget(p);
	kfree(p);
}

void process_forget(struct process *p) {
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
	sysexit(proc->regs);
}

_Noreturn void process_switch_any(void) {
	if (process_current && process_current->state == PS_RUNNING)
		process_switch(process_current);

	struct process *found = process_find(PS_RUNNING);
	if (found) process_switch(found);

	cpu_pause();
	process_switch_any();
}

struct process *process_next(struct process *p) {
	/* is a weird depth-first search, the search order is:
	 *         1
	 *        / \
	 *       2   5
	 *      /|   |\
	 *     3 4   6 7
	 */
	if (!p)	return NULL;
	if (p->child)	return p->child;
	if (p->sibling)	return p->sibling;

	/* looking at the diagram above - we're at 4, want to find 5 */
	while (!p->sibling) {
		p = p->parent;
		if (!p) return NULL;
	}
	return p->sibling;
}

struct process *process_find(enum process_state target) {
	for (struct process *p = process_first; p; p = process_next(p)) {
		if (p->state == target) return p;
	}
	return NULL;
}

handle_t process_find_free_handle(struct process *proc, handle_t start_at) {
	// TODO start_at is a bit of a hack
	handle_t handle;
	for (handle = start_at; handle < HANDLE_MAX; handle++) {
		if (proc->handles[handle] == NULL)
			break;
	}
	if (handle >= HANDLE_MAX) handle = -1;
	return handle;
}

struct handle *process_handle_get(struct process *p, handle_t id) {
	if (id < 0 || id >= HANDLE_MAX) return NULL;
	return p->handles[id];
}

void process_transition(struct process *p, enum process_state state) {
	enum process_state last = p->state;
	p->state = state;
	switch (state) {
		case PS_RUNNING:
			assert(last != PS_DEAD);
			break;
		case PS_DEAD:
			// see process_kill
			break;
		case PS_WAITS4CHILDDEATH:
		case PS_WAITS4FS:
		case PS_WAITS4REQUEST:
		case PS_WAITS4PIPE:
			assert(last == PS_RUNNING);
			break;

		case PS_LAST:
			panic_invalid_state();
	}
}

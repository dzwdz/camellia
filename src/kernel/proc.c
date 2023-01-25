#include <camellia/errno.h>
#include <camellia/flags.h>
#include <kernel/arch/generic.h>
#include <kernel/execbuf.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/procfs.h>
#include <shared/mem.h>
#include <stdint.h>

static struct process *process_first = NULL;
static struct process *process_forgotten = NULL; /* linked list */
struct process *process_current;

static uint32_t next_pid = 1;


/** Removes a process from the process tree. */
static void process_forget(struct process *p);
static void process_free_forgotten(void);
static _Noreturn void process_switch(struct process *proc);


struct process *process_seed(void *data, size_t datalen) {
	assert(!process_first);
	process_first = kzalloc(sizeof *process_first);
	process_first->state = PS_RUNNING;
	process_first->pages = pagedir_new();
	process_first->mount = vfs_mount_seed();
	process_first->globalid = next_pid++;
	process_first->cid = 1;
	process_first->nextcid = 1;
	process_first->_handles = kzalloc(sizeof(struct handle) * HANDLE_MAX);

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
	struct process *child = kzalloc(sizeof *child);

	if (flags & FORK_SHAREMEM) {
		if (!parent->pages_refcount) {
			parent->pages_refcount = kmalloc(sizeof *parent->pages_refcount);
			*parent->pages_refcount = 1;
		}
		*parent->pages_refcount += 1;
		child->pages_refcount = parent->pages_refcount;
		child->pages = parent->pages;
	} else {
		child->pages = pagedir_copy(parent->pages);
	}
	child->regs  = parent->regs;
	child->state = parent->state;
	assert(child->state == PS_RUNNING); // not copying the state union
	child->intr_fn = parent->intr_fn;

	child->noreap  = (flags & FORK_NOREAP) > 0;

	child->sibling = parent->child;
	child->child   = NULL;
	child->parent  = parent;
	parent->child  = child;

	if (parent->nextcid == 0)
		panic_unimplemented();
	child->cid = parent->nextcid++;
	child->nextcid = 1;
	child->globalid = next_pid++;

	if ((flags & FORK_NEWFS) == 0 && parent->controlled) {
		child->controlled = parent->controlled;
		assert(child->controlled->provhcnt);
		child->controlled->provhcnt++;
	}

	child->mount = parent->mount;
	assert(child->mount);
	child->mount->refs++;

	if (flags & FORK_SHAREHANDLE) {
		if (!parent->handles_refcount) {
			parent->handles_refcount = kmalloc(sizeof *parent->handles_refcount);
			*parent->handles_refcount = 1;
		}
		*parent->handles_refcount += 1;
		child->handles_refcount = parent->handles_refcount;
		child->_handles = parent->_handles;
	} else {
		child->_handles = kzalloc(sizeof(struct handle) * HANDLE_MAX);
		for (handle_t h = 0; h < HANDLE_MAX; h++) {
			child->_handles[h] = parent->_handles[h];
			if (child->_handles[h])
				child->_handles[h]->refcount++;
		}
	}

	return child;
}

/* meant to be used with p->*_refcount */
static bool unref(uint64_t *refcount) {
	if (!refcount) return true;
	assert(*refcount != 0);
	*refcount -= 1;
	if (*refcount == 0) {
		kfree(refcount);
		return true;
	}
	return false;
}

void process_kill(struct process *p, int ret) {
	if (proc_alive(p)) {
		if (p->controlled) {
			// TODO vfs_backend_user_handlerdown
			assert(p->controlled->provhcnt > 0);
			if (p->controlled->user.handler == p) {
				assert(p->state == PS_WAITS4REQUEST);
				p->controlled->user.handler = NULL;
			}
			vfs_backend_refdown(p->controlled, false);
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

		if (p->state == PS_WAITS4TIMER) {
			timer_deschedule(p);
		}

		if (unref(p->handles_refcount)) {
			for (handle_t hid = 0; hid < HANDLE_MAX; hid++)
				process_handle_close(p, hid);
			kfree(p->_handles);
		}
		p->_handles = NULL;

		vfs_mount_remref(p->mount);
		p->mount = NULL;

		process_transition(p, PS_DYING);
		p->death_msg = ret;

		/* tombstone TOREAP children */
		for (struct process *it = p->child; it; ) {
			struct process *sibling = it->sibling;
			if (it->state == PS_TOREAP) {
				process_tryreap(it);
			}
			it = sibling;
		}

		if (p->execbuf.buf) {
			kfree(p->execbuf.buf);
			p->execbuf.buf = NULL;
		}

		if (unref(p->pages_refcount)) {
			pagedir_free(p->pages);
		}
		p->pages = NULL;
	}

	if (p->state == PS_DYING) {
		if (p->parent && proc_alive(p->parent)) {
			process_transition(p, PS_TOREAP);
		} else {
			process_transition(p, PS_TOMBSTONE);
		}
	}
	if (p == process_first && p->child) {
		_panic("init killed prematurely");
	}
	process_tryreap(p);

	if (p == process_first) {
		process_free_forgotten();
		shutdown();
	}
}

void process_filicide(struct process *parent, int ret) {
	/* Kill deeper descendants. */
	struct process *child, *child2;
	for (child = parent->child; child; child = child2) {
		child2 = child->sibling;

		// O(n^2), but doable in linear time
		while (child->child) {
			struct process *p = child->child;
			while (p->child) p = p->child;
			p->noreap = true;
			process_kill(p, ret);
		}

		if (proc_alive(child)) {
			process_kill(child, ret);
		}
		child = NULL;
	}
}

void process_tryreap(struct process *dead) {
	struct process *parent;
	assert(dead && !proc_alive(dead));
	parent = dead->parent;
	if (parent) assert(parent->child);

	if (dead->state == PS_TOREAP) {
		if (!dead->noreap && parent && proc_alive(parent) && parent->state != PS_DYING) {
			/* reapable? */
			if (parent->state != PS_WAITS4CHILDDEATH) {
				return; /* don't reap yet */
			}
			regs_savereturn(&parent->regs, dead->death_msg);
			process_transition(parent, PS_RUNNING);
		}
		/* can't be reaped anymore */
		process_transition(dead, PS_TOMBSTONE);
		dead->noreap = true;
	}

	assert(dead->state == PS_TOMBSTONE);
	for (struct process *p = dead->child; p; p = p->sibling) {
		assert(p->state != PS_TOREAP);
	}

	if (dead->child) {
		struct process *p = dead->child;
		while (p->child) p = p->child;
		if (p->state == PS_TOREAP) {
			assert(proc_alive(p->parent));
		} else {
			assert(proc_alive(p));
		}
		return; /* keep the tombstone */
	}

	handle_close(dead->specialh.procfs);
	assert(dead->refcount == 0);
	if (parent) { /* not applicable to init */
		process_forget(dead);

		// TODO force gcc to optimize the tail call here
		if (!proc_alive(parent)) {
			process_tryreap(parent);
		}
	}
}

void process_intr(struct process *p) {
	if (!p->intr_fn) return;

	/* save old rsp,rip */
	struct intr_data d;
	void __user *sp = p->regs.rsp - 128 - sizeof(d);
	d.ip = (void __user *)p->regs.rcx;
	d.sp = p->regs.rsp;
	pcpy_to(p, sp, &d, sizeof(d));

	/* switch to intr handler */
	p->regs.rcx = (uint64_t)p->intr_fn;
	p->regs.rsp = sp;
}

/** Removes a process from the process tree. */
static void process_forget(struct process *p) {
	assert(p->parent);
	assert(p->parent->child);
	assert(!p->child);

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

	p->parent = NULL;
	p->sibling = process_forgotten;
	process_forgotten = p;
	process_transition(p, PS_FORGOTTEN);
}

static void process_free_forgotten(void) {
	while (process_forgotten) {
		struct process *p = process_forgotten;
		process_forgotten = p->sibling;

		process_transition(p, PS_FREED);
		kfree(p);
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
	/* At this point there will be no leftover pointers to forgotten
	 * processes on the stack, so it's safe to free them. */
	process_free_forgotten();

	for (;;) {
		if (process_current && process_current->state == PS_RUNNING)
			process_switch(process_current);

		for (struct process *p = process_first; p; p = process_next(p, NULL)) {
			if (p->state == PS_RUNNING)
				process_switch(p);
		}

		cpu_pause();
	}
}

struct process *process_next(struct process *p, struct process *root) {
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
		if (root) assert(p);
		if (!p || p == root) return NULL;
	}
	return p->sibling;
}

handle_t process_find_free_handle(struct process *proc, handle_t start_at) {
	for (handle_t hid = start_at; hid < HANDLE_MAX; hid++) {
		if (proc->_handles[hid] == NULL)
			return hid;
	}
	return -1;
}

struct handle *process_handle_get(struct process *p, handle_t id) {
	if (id == HANDLE_NULLFS) {
		static struct handle h = (struct handle){
			.type = HANDLE_FS_FRONT,
			.backend = NULL,
			.refcount = 2, /* never free */
		};
		return &h;
	} else if (id == HANDLE_PROCFS) {
		if (!p->specialh.procfs) {
			struct handle *h = kmalloc(sizeof *h);
			*h = (struct handle){
				.type = HANDLE_FS_FRONT,
				.backend = procfs_backend(p),
				.refcount = 1,
			};
			p->specialh.procfs = h;
		}
		return p->specialh.procfs;
	} else if (0 <= id && id < HANDLE_MAX) {
		return p->_handles[id];
	} else {
		return NULL;
	}
}

handle_t process_handle_init(struct process *p, enum handle_type type, struct handle **hs) {
	handle_t hid = process_find_free_handle(p, 1);
	if (hid < 0) return -1;
	p->_handles[hid] = handle_init(type);
	if (hs) *hs = p->_handles[hid];
	return hid;
}

handle_t process_handle_dup(struct process *p, handle_t from, handle_t to) {
	struct handle *fromh, **toh;

	if (to < 0) {
		to = process_find_free_handle(p, 0);
		if (to < 0) return -EMFILE;
	} else if (to >= HANDLE_MAX) {
		return -EBADF;
	}

	if (to == from) return to;
	toh = &p->_handles[to];
	fromh = process_handle_get(p, from);

	if (*toh) handle_close(*toh);
	*toh = fromh;
	if (fromh) fromh->refcount++;

	return to;
}

struct handle *process_handle_take(struct process *p, handle_t hid) {
	if (hid < 0 || hid >= HANDLE_MAX) {
		return process_handle_get(p, hid);
	}
	struct handle *h = p->_handles[hid];
	p->_handles[hid] = NULL;
	return h;
}

handle_t process_handle_put(struct process *p, struct handle *h) {
	assert(h);
	handle_t hid = process_find_free_handle(p, 1);
	if (hid < 0) {
		handle_close(h);
		return hid;
	}
	p->_handles[hid] = h;
	return hid;
}

void process_transition(struct process *p, enum process_state state) {
	assert(p->state != PS_FREED);
	if (state == PS_FREED) {
		assert(p->state == PS_FORGOTTEN);
	} else if (state == PS_FORGOTTEN) {
		assert(p->state == PS_TOMBSTONE);
	} else if (state == PS_TOMBSTONE) {
		assert(p->state == PS_TOREAP || p->state == PS_DYING);
	} else if (state == PS_TOREAP) {
		assert(p->state == PS_DYING);

		assert(!p->parent || proc_alive(p->parent));
		for (struct process *it = p->child; it; it = it->sibling) {
			assert(p->state != PS_TOREAP);
		}
	} else if (state != PS_RUNNING && state != PS_DYING) {
		assert(p->state == PS_RUNNING);
	}
	p->state = state;
}

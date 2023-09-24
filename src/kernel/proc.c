#include <camellia/errno.h>
#include <camellia/flags.h>
#include <kernel/arch/generic.h>
#include <kernel/execbuf.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>
#include <stdint.h>

static Proc *proc_first = NULL;
static Proc *dead_leaves = NULL; /* linked list */
Proc *proc_cur;

static uint32_t next_pid = 1;

static void proc_prune_leaves(void);

Proc *proc_seed(void *data, size_t datalen) {
	assert(!proc_first);
	proc_first = kzalloc(sizeof *proc_first);
	proc_first->state = PS_RUNNING;
	proc_first->pages = pagedir_new();
	proc_first->mount = vfs_mount_seed();
	proc_first->globalid = next_pid++;
	proc_first->_handles = kzalloc(sizeof(Handle) * HANDLE_MAX);

	proc_first->pns = proc_first;
	proc_first->localid = 1;
	proc_first->nextlid = 2;

	// map .shared
	extern char _shared_len;
	for (size_t p = 0; p < (size_t)&_shared_len; p += PAGE_SIZE)
		pagedir_map(proc_first->pages, (userptr_t)p, (void*)p, false, true);

	// map the init module as rw
	void __user *init_base = (userptr_t)0x200000;
	for (uintptr_t off = 0; off < datalen; off += PAGE_SIZE)
		pagedir_map(proc_first->pages, init_base + off, data + off, true, true);

	proc_first->regs.rcx = (uintptr_t)init_base; // SYSRET jumps to %rcx
	// proc_first->regs.r11 |= 1<<9; /* enable interrupts */

	return proc_first;
}

Proc *proc_fork(Proc *parent, int flags) {
	Proc *child = kzalloc(sizeof *child);

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

	if (next_pid == 0) {
		panic_unimplemented();
	}
	child->globalid = next_pid++;

	child->pns = parent->pns;
	if (child->pns->nextlid == 0) {
		panic_unimplemented();
	}
	child->localid = child->pns->nextlid++;


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
		child->_handles = kzalloc(sizeof(Handle) * HANDLE_MAX);
		for (hid_t h = 0; h < HANDLE_MAX; h++) {
			child->_handles[h] = parent->_handles[h];
			if (child->_handles[h])
				child->_handles[h]->refcount++;
		}
	}

	return child;
}

bool proc_ns_contains(Proc *ns, Proc *proc) {
	/* a namespace contains all the processes with ->ns == ns and all their
	 * direct children */
	if (ns == proc) return true;
	if (proc->parent == NULL) return false;
	return proc->parent->pns == ns;
}

uint32_t proc_ns_id(Proc *ns, Proc *proc) {
	if (proc == ns) {
		return 1;
	} else {
		if (proc->pns == proc) {
			assert(proc->parent->pns == ns);
		} else {
			assert(proc->pns == ns);
		}
		return proc->localid;
	}
}

Proc *proc_ns_byid(Proc *ns, uint32_t id) {
	assert(ns->pns == ns);
	for (Proc *it = ns; it; it = proc_ns_next(ns, it)) {
		if (proc_ns_id(ns, it) == id) {
			return it;
		}
	}
	return NULL;
}

Proc *proc_ns_next(Proc *ns, Proc *p) {
	Proc *ret = NULL;
	/* see comments in proc_next */

	if (!p) goto end;
	/* descend into children who own their own namespace, but no further */
	if (p->child && proc_ns_contains(ns, p->child)) {
		ret = p->child;
		goto end;
	}
	// TODO diverged from proc_next, integrate this fix into it
	// also once you do that do regression tests - this behaviour is buggy
	if (p == ns) {
		/* don't escape the root */
		goto end;
	}
	while (!p->sibling) {
		p = p->parent;
		assert(p);
		if (p == ns) goto end;
	}
	ret = p->sibling;

end:
	if (ret != NULL) {
		assert(proc_ns_contains(ns, ret));
	}
	return ret;
}

void proc_ns_create(Proc *proc) {
	// TODO test this. lots of fucky behaviour can happen here
	// TODO document process namespaces
	Proc *old = proc->pns;
	if (old == proc) return;
	proc->pns = proc;
	proc->nextlid = 2;
	for (Proc *it = proc; it; ) {
		if (it != proc) {
			if (proc->nextlid < it->localid + 1) {
				proc->nextlid = it->localid + 1;
			}
			if (it->pns == old) {
				it->pns = proc;
			} else {
				assert(it->pns == it);
			}
		}

		/* analogous to proc_ns_next - which can't be used directly as it gets
		 * confused by changing namespaces */

		/* descend into children who own their own namespace, but no further */
		if (it->child && (proc_ns_contains(proc, it->child) || proc_ns_contains(old, it->child))) {
			it = it->child;
			continue;
		}
		if (it == proc) {
			break;
		}
		while (!it->sibling) {
			it = it->parent;
			if (it == proc) break;
			assert(it);
		}
		it = it->sibling;
	}
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

void proc_kill(Proc *p, int ret) {
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
			Proc **iter = &p->waits4pipe.pipe->pipe.queued;
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
			for (hid_t hid = 0; hid < HANDLE_MAX; hid++)
				proc_handle_close(p, hid);
			kfree(p->_handles);
		}
		p->_handles = NULL;

		vfs_mount_remref(p->mount);
		p->mount = NULL;

		proc_setstate(p, PS_DYING);
		p->death_msg = ret;

		/* tombstone TOREAP children */
		for (Proc *it = p->child; it; ) {
			Proc *sibling = it->sibling;
			if (it->state == PS_TOREAP) {
				proc_tryreap(it);
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
			proc_setstate(p, PS_TOREAP);
		} else {
			proc_setstate(p, PS_TOMBSTONE);
		}
	}
	if (p == proc_first) {
		proc_prune_leaves();
		if (p->child) {
			_panic("init killed prematurely");
		}
		shutdown();
	} else {
		proc_tryreap(p);
	}
}

void proc_tryreap(Proc *dead) {
	Proc *parent;
	assert(dead && !proc_alive(dead));
	parent = dead->parent;
	if (parent) assert(parent->child);

	if (dead->state == PS_TOREAP) {
		if (!dead->noreap && parent && proc_alive(parent) && parent->state != PS_DYING) {
			/* reapable? */
			if (parent->state != PS_WAITS4CHILDDEATH) {
				return; /* don't reap yet */
			}
			uint32_t pid = proc_ns_id(parent->pns, dead);
			if (parent->awaited_death.pid && parent->awaited_death.pid != pid) {
				return; /* we're not The One */
			}
			regs_savereturn(&parent->regs, pid);
			if (parent->awaited_death.out) {
				struct sys_wait2 __user *out = parent->awaited_death.out;
				struct sys_wait2 data;
				memset(&data, 0, sizeof data);
				data.status = dead->death_msg;
				pcpy_to(parent, out, &data, sizeof data);
			}
			proc_setstate(parent, PS_RUNNING);
		}
		/* can't be reaped anymore */
		proc_setstate(dead, PS_TOMBSTONE);
		dead->noreap = true;
	}

	if (dead->child) {
		assert(dead->state == PS_TOMBSTONE);
		for (Proc *p = dead->child; p; p = p->sibling) {
			assert(p->state != PS_TOREAP);
		}

		Proc *p = dead->child;
		while (p->child) {
			p = p->child;
		}
		if (p->state == PS_TOREAP) {
			assert(proc_alive(p->parent));
		} else {
			assert(proc_alive(p) || p->state == PS_DEADLEAF);
		}

		return; /* keep the tombstone */
	}

	if (!parent) return; /* not applicable to init */
	assert(dead->refcount == 0);

	if (dead->state == PS_TOMBSTONE) {
		proc_setstate(dead, PS_DEADLEAF);
		/* not altering the process tree right now to prevent breaking tree
		 * traversals */
		dead->deadleaf_next = dead_leaves;
		dead_leaves = dead;
	} else {
		assert(dead->state == PS_DEADLEAF);
	}
}

void proc_tryintr(Proc *p) {
	if (p->state == PS_WAITS4TIMER) {
		timer_deschedule(p);
	}
}

void proc_intr(Proc *p, const char *buf, size_t len) {
	assert(buf != NULL || len == 0);

	if (4 <= len && memcmp(buf, "kill", 4) == 0) {
		proc_kill(p, EINTR);
		return;
	}

	if (!p->intr_fn) return;
	proc_tryintr(p);

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

static void proc_prune_leaves(void) {
	while (dead_leaves) {
		Proc *p = dead_leaves;
		Proc *parent = p->parent;

		dead_leaves = p->deadleaf_next;
		assert(p->state == PS_DEADLEAF);

		assert(p->parent);
		assert(p->parent->child);
		assert(!p->child);
		if (p->parent->child == p) {
			p->parent->child = p->sibling;
		} else {
			// this would be simpler if siblings were a doubly linked list
			Proc *prev = p->parent->child;
			while (prev->sibling != p) {
				prev = prev->sibling;
				assert(prev);
			}
			prev->sibling = p->sibling;
		}

		proc_setstate(p, PS_FREED);
		kfree(p);

		if (!proc_alive(parent)) {
			proc_tryreap(parent);
		}
	}
}

_Noreturn void proc_switch_any(void) {
	/* At this point there will be no leftover pointers to forgotten
	 * processes on the stack, so it's safe to free them. */
	proc_prune_leaves();

	for (;;) {
		Proc *p = NULL;
		if (proc_cur && proc_cur->state == PS_RUNNING) {
			p = proc_cur;
		} else {
			for (p = proc_first; p; p = proc_next(p, NULL)) {
				if (p->state == PS_RUNNING) {
					break;
				}
			}
		}
		if (p) {
			assert(p->state == PS_RUNNING);
			proc_cur = p;
			pagedir_switch(p->pages);
			if (p->execbuf.buf) {
				execbuf_run(p);
			} else {
				sysexit(p->regs);
			}
		} else {
			cpu_pause();
		}
	}
}

Proc *proc_next(Proc *p, Proc *root) {
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

hid_t proc_find_free_handle(Proc *proc, hid_t start_at) {
	if (start_at < 0) {
		start_at = 0;
	}
	for (hid_t hid = start_at; hid < HANDLE_MAX; hid++) {
		if (proc->_handles[hid] == NULL)
			return hid;
	}
	return -1;
}

Handle *proc_handle_get(Proc *p, hid_t id) {
	if (id == HANDLE_NULLFS) {
		static Handle h = (Handle){
			.type = HANDLE_FS_FRONT,
			.backend = NULL,
			.refcount = 2, /* never free */
		};
		return &h;
	} else if (0 <= id && id < HANDLE_MAX) {
		return p->_handles[id];
	} else {
		return NULL;
	}
}

hid_t proc_handle_init(Proc *p, enum handle_type type, Handle **hs) {
	hid_t hid = proc_find_free_handle(p, 1);
	if (hid < 0) return -1;
	p->_handles[hid] = handle_init(type);
	if (hs) *hs = p->_handles[hid];
	return hid;
}

hid_t proc_handle_dup(Proc *p, hid_t from, hid_t to, int flags) {
	Handle *fromh, **toh;

	if (to < 0 || (flags & DUP_SEARCH)) {
		to = proc_find_free_handle(p, to);
		if (to < 0) return -EMFILE;
	} else if (to >= HANDLE_MAX) {
		return -EBADF;
	}

	if (to == from) return to;
	toh = &p->_handles[to];
	fromh = proc_handle_get(p, from);

	if (*toh) handle_close(*toh);
	*toh = fromh;
	if (fromh) fromh->refcount++;

	return to;
}

Handle *proc_hid_take(Proc *p, hid_t hid) {
	if (hid < 0 || hid >= HANDLE_MAX) {
		return proc_handle_get(p, hid);
	}
	Handle *h = p->_handles[hid];
	p->_handles[hid] = NULL;
	return h;
}

hid_t proc_handle_put(Proc *p, Handle *h) {
	assert(h);
	hid_t hid = proc_find_free_handle(p, 1);
	if (hid < 0) {
		handle_close(h);
		return hid;
	}
	p->_handles[hid] = h;
	return hid;
}

void proc_setstate(Proc *p, enum proc_state state) {
	assert(p->state != PS_FREED);
	if (state == PS_FREED) {
		assert(p->state == PS_DEADLEAF);
	} else if (state == PS_DEADLEAF) {
		assert(p->state == PS_TOMBSTONE);
	} else if (state == PS_TOMBSTONE) {
		assert(p->state == PS_TOREAP || p->state == PS_DYING);
	} else if (state == PS_TOREAP) {
		assert(p->state == PS_DYING);

		assert(!p->parent || proc_alive(p->parent));
		for (Proc *it = p->child; it; it = it->sibling) {
			assert(p->state != PS_TOREAP);
		}
	} else if (state != PS_RUNNING && state != PS_DYING) {
		assert(p->state == PS_RUNNING);
	}
	p->state = state;
}

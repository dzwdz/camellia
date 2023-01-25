#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/util.h>

static void pipe_trytransfer(Handle *h);

void pipe_joinqueue(Handle *h, Proc *proc, void __user *pbuf, size_t pbuflen) {
	assert(h && h->type == HANDLE_PIPE);
	assert(h->readable ^ h->writeable);
	if (!h->pipe.sister) {
		regs_savereturn(&proc->regs, -1);
		return;
	}

	Proc **slot = &h->pipe.queued;
	while (*slot) {
		assert((*slot)->state == PS_WAITS4PIPE);
		slot = &((*slot)->waits4pipe.next);
	}

	proc_setstate(proc, PS_WAITS4PIPE);
	*slot = proc;
	proc->waits4pipe.pipe = h;
	proc->waits4pipe.buf = pbuf;
	proc->waits4pipe.len = pbuflen;
	proc->waits4pipe.next = NULL;
	pipe_trytransfer(h);
}

static void pipe_trytransfer(Handle *h) {
	Proc *rdr, *wtr;
	int len;
	assert(h && h->type == HANDLE_PIPE);
	assert(h->readable ^ h->writeable);
	if (!h->pipe.sister) {
		assert(!h->pipe.queued);
		return;
	}

	rdr = h->readable ? h->pipe.queued : h->pipe.sister->pipe.queued;
	wtr = h->writeable ? h->pipe.queued : h->pipe.sister->pipe.queued;
	if (!(rdr && wtr)) return;
	assert(rdr->state == PS_WAITS4PIPE);
	assert(wtr->state == PS_WAITS4PIPE);

	len = min(rdr->waits4pipe.len, wtr->waits4pipe.len);
	len = pcpy_bi(
		rdr, rdr->waits4pipe.buf,
		wtr, wtr->waits4pipe.buf,
		len
	);
	h->pipe.queued = h->pipe.queued->waits4pipe.next;
	h->pipe.sister->pipe.queued = h->pipe.sister->pipe.queued->waits4pipe.next;
	proc_setstate(rdr, PS_RUNNING);
	proc_setstate(wtr, PS_RUNNING);
	regs_savereturn(&rdr->regs, len);
	regs_savereturn(&wtr->regs, len);
}

void pipe_invalidate_end(Handle *h) {
	Proc *p = h->pipe.queued;
	while (p) {
		assert(p->state == PS_WAITS4PIPE);
		proc_setstate(p, PS_RUNNING);
		regs_savereturn(&p->regs, -1);
		p = p->waits4pipe.next;
	}
	h->pipe.queued = NULL;
}

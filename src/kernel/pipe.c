#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/util.h>

static void pipe_trytransfer(struct handle *h);

void pipe_joinqueue(struct handle *h, struct process *proc, void __user *pbuf, size_t pbuflen) {
	assert(h && h->type == HANDLE_PIPE);
	assert(h->readable ^ h->writeable);
	if (!h->pipe.sister) {
		regs_savereturn(&proc->regs, -1);
		return;
	}

	struct process **slot = &h->pipe.queued;
	while (*slot) {
		assert((*slot)->state == PS_WAITS4PIPE);
		slot = &((*slot)->waits4pipe.next);
	}

	process_transition(proc, PS_WAITS4PIPE);
	*slot = proc;
	proc->waits4pipe.pipe = h;
	proc->waits4pipe.buf = pbuf;
	proc->waits4pipe.len = pbuflen;
	proc->waits4pipe.next = NULL;
	pipe_trytransfer(h);
}

static void pipe_trytransfer(struct handle *h) {
	struct process *rdr, *wtr;
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
	process_transition(rdr, PS_RUNNING);
	process_transition(wtr, PS_RUNNING);
	regs_savereturn(&rdr->regs, len);
	regs_savereturn(&wtr->regs, len);
}

void pipe_invalidate_end(struct handle *h) {
	struct process *p = h->pipe.queued;
	while (p) {
		assert(p->state == PS_WAITS4PIPE);
		process_transition(p, PS_RUNNING);
		regs_savereturn(&p->regs, -1);
		p = p->waits4pipe.next;
	}
	h->pipe.queued = NULL;
}

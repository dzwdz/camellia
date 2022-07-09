#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/util.h>

bool pipe_joinqueue(struct handle *h, bool wants_write,
		struct process *proc, void __user *pbuf, size_t pbuflen)
{
	assert(h && h->type == HANDLE_PIPE);
	if (wants_write == h->pipe.write_end) return false;
	if (!h->pipe.sister) return false;
	if (h->pipe.queued) {
		assert(h->pipe.queued->state == PS_WAITS4PIPE);
		panic_unimplemented();
	}

	process_transition(proc, PS_WAITS4PIPE);
	h->pipe.queued = proc;
	proc->waits4pipe.pipe = h;
	proc->waits4pipe.buf = pbuf;
	proc->waits4pipe.len = pbuflen;
	return true;
}

void pipe_trytransfer(struct handle *h) {
	struct process *rdr, *wtr;
	int len;
	assert(h);
	if (!h->pipe.sister) {
		assert(!h->pipe.queued);
		return;
	}

	rdr = h->pipe.write_end ? h->pipe.sister->pipe.queued : h->pipe.queued;
	wtr = h->pipe.write_end ? h->pipe.queued : h->pipe.sister->pipe.queued;

	if (!(rdr && wtr)) return;
	assert(rdr->state == PS_WAITS4PIPE);
	assert(wtr->state == PS_WAITS4PIPE);

	len = min(rdr->waits4pipe.len, wtr->waits4pipe.len);

	if (!virt_cpy(
			rdr->pages, rdr->waits4pipe.buf,
			wtr->pages, wtr->waits4pipe.buf, len))
	{
		panic_unimplemented();
	}
	process_transition(rdr, PS_RUNNING);
	process_transition(wtr, PS_RUNNING);
	h->pipe.queued = NULL;
	h->pipe.sister->pipe.queued = NULL;
	regs_savereturn(&rdr->regs, len);
	regs_savereturn(&wtr->regs, len);
}

void pipe_invalidate_end(struct handle *h) {
	struct process *p = h->pipe.queued;
	if (p) {
		process_transition(p, PS_RUNNING);
		regs_savereturn(&p->regs, -1);
	}
	h->pipe.queued = NULL;
}

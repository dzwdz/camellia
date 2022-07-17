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
	return true;
}

void pipe_trytransfer(struct handle *h) {
	struct process *rdr, *wtr;
	struct virt_cpy_error cpyerr;
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

	virt_cpy(
			rdr->pages, rdr->waits4pipe.buf,
			wtr->pages, wtr->waits4pipe.buf,
			len, &cpyerr);
	if (cpyerr.read_fail || cpyerr.write_fail)
		panic_unimplemented();
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

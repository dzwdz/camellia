#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/util.h>

void pipe_joinqueue(struct handle *h, bool wants_write,
		struct process *proc, void __user *pbuf, size_t pbuflen)
{
	struct process **slot = wants_write ? &h->pipe.reader : &h->pipe.writer;
	if (*slot) {
		assert((*slot)->state == PS_WAITS4PIPE);
		panic_unimplemented();
	}

	process_transition(proc, PS_WAITS4PIPE);
	*slot = proc;
	proc->waits4pipe.pipe = h;
	proc->waits4pipe.buf = pbuf;
	proc->waits4pipe.len = pbuflen;
}

void pipe_trytransfer(struct handle *h) {
	struct process *rdr = h->pipe.reader, *wtr = h->pipe.writer;
	int len;
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
	h->pipe.reader = NULL;
	h->pipe.writer = NULL;
	regs_savereturn(&rdr->regs, len);
	regs_savereturn(&wtr->regs, len);
}

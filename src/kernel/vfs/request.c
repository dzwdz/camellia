#include <camellia/errno.h>
#include <camellia/flags.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static void vfs_backend_user_accept(VfsReq *req);

void vfsreq_create(VfsReq req_) {
	VfsReq *req;
	if (req_.caller) {
		proc_setstate(req_.caller, PS_WAITS4FS);
		if (!req_.caller->reqslot)
			req_.caller->reqslot = kmalloc(sizeof *req);
		req = req_.caller->reqslot;
		/* (re)using a single allocation for all request a process makes */
	} else {
		req = kmalloc(sizeof *req);
	}
	memcpy(req, &req_, sizeof *req);
	if (req->backend) {
		assert(req->backend->usehcnt);
		req->backend->usehcnt++;
	}

	if (req->type == VFSOP_OPEN && !(req->flags & OPEN_WRITE) && (req->flags & OPEN_CREATE)) {
		vfsreq_finish_short(req, -EINVAL);
		return;
	}

	// TODO if i add a handle field to vfs_request, check ->readable ->writeable here

	if (req->backend && req->backend->provhcnt) {
		VfsReq **iter = &req->backend->queue;
		while (*iter != NULL) // find free spot in queue
			iter = &(*iter)->queue_next;
		*iter = req;
		vfs_backend_tryaccept(req->backend);
	} else {
		vfsreq_finish_short(req, -1);
	}
}

void vfsreq_finish(VfsReq *req, char __user *stored, long ret,
		int flags, Proc *handler)
{
	if (req->type == VFSOP_OPEN && ret >= 0) {
		Handle *h;
		if (!(flags & FSR_DELEGATE)) {
			/* default behavior - create a new handle for the file, wrap the id */
			h = handle_init(HANDLE_FILE);
			h->backend = req->backend;
			req->backend->usehcnt++;
			h->file_id = stored;
			h->readable = OPEN_READABLE(req->flags);
			h->writeable = OPEN_WRITEABLE(req->flags);
		} else {
			/* delegating - moving a handle to the caller */
			assert(handler);
			h = proc_hid_take(handler, ret);
			// TODO don't ignore OPEN_RO
		}

		if (h) {
			// TODO write tests for caller getting killed while opening a file
			if (!req->caller) panic_unimplemented();
			ret = proc_handle_put(req->caller, h);
			if (ret < 0) ret = -EMFILE;
		} else {
			ret = -1;
		}
	}

	if (req->type == VFSOP_READ && ret >= 0) {
		assert((size_t)ret <= req->output.len);
	}

	if (req->input.kern)
		kfree(req->input.buf_kern);

	if (req->backend)
		vfs_backend_refdown(req->backend, true);

	if (req->caller) {
		assert(req->caller->state == PS_WAITS4FS);
		regs_savereturn(&req->caller->regs, ret);
		proc_setstate(req->caller, PS_RUNNING);
	} else {
		kfree(req);
	}
}

void vfs_backend_tryaccept(VfsBackend *backend) {
	VfsReq *req = backend->queue;
	if (!req) return;
	if (backend->is_user && !backend->user.handler) return;

	backend->queue = req->queue_next;
	if (backend->is_user) {
		vfs_backend_user_accept(req);
	} else {
		assert(backend->kern.accept);
		backend->kern.accept(req);
	}
}

static void vfs_backend_user_accept(VfsReq *req) {
	Proc *handler;
	struct ufs_request res = {0};
	int len;

	assert(req && req->backend && req->backend->user.handler);
	handler = req->backend->user.handler;
	assert(handler->state == PS_WAITS4REQUEST);

	// the pcpy calls aren't present in all kernel backends
	// it's a way to tell apart kernel and user backends apart
	// TODO check validity of memory regions somewhere else

	if (req->input.buf) {
		__user void *buf = handler->awaited_req.buf;
		len = min(req->input.len, handler->awaited_req.max_len);
		if (req->input.kern) {
			pcpy_to(handler, buf, req->input.buf_kern, len);
		} else {
			len = pcpy_bi(
				handler, buf,
				req->caller, req->input.buf, len
			);
		}
	} else {
		len = req->output.len;
	}

	res.len      = len;
	res.capacity = req->output.len;
	res.id       = req->id;
	res.offset   = req->offset;
	res.flags    = req->flags;
	res.op       = req->type;

	if (pcpy_to(handler, handler->awaited_req.res, &res, sizeof res) < sizeof(res))
	{
		panic_unimplemented();
	}

	Handle *h;
	hid_t hid = proc_handle_init(handler, HANDLE_FS_REQ, &h);
	if (hid < 0) panic_unimplemented();
	h->req = req;
	proc_setstate(handler, PS_RUNNING);
	regs_savereturn(&handler->regs, hid);
	req->backend->user.handler = NULL;
	return;
}

void vfs_backend_refdown(VfsBackend *b, bool use) {
	size_t *field = use ? &b->usehcnt : &b->provhcnt;
	assert(b);
	assert(0 < *field);
	*field -= 1;

	if (b->provhcnt == 0 && use == false) {
		VfsReq *q = b->queue;
		while (q) {
			VfsReq *q2 = q->queue_next;
			vfsreq_finish_short(q, -1);
			q = q2;
		}
		b->queue = NULL;
	}
	if (b->usehcnt == 0 && use == true) {
		if (!b->is_user && b->kern.cleanup) {
			b->kern.cleanup(b);
		}
		if (b->is_user && b->user.handler) {
			Proc *p = b->user.handler;
			b->user.handler = NULL;
			assert(p->state == PS_WAITS4REQUEST);
			regs_savereturn(&p->regs, -EPIPE);
			proc_setstate(p, PS_RUNNING);
		}
	}
	if (b->usehcnt == 0 && b->provhcnt == 0) {
		assert(!b->queue);
		kfree(b);
	}
}

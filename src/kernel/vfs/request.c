#include <camellia/errno.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

void vfsreq_create(struct vfs_request req_) {
	struct vfs_request *req;
	if (req_.caller) {
		process_transition(req_.caller, PS_WAITS4FS);
		if (!req_.caller->reqslot)
			req_.caller->reqslot = kmalloc(sizeof *req);
		req = req_.caller->reqslot;
		/* (re)using a single allocation for all request a process makes */
	} else {
		req = kmalloc(sizeof *req);
	}
	memcpy(req, &req_, sizeof *req);

	if (req->backend)
		req->backend->refcount++;

	if (req->backend && req->backend->potential_handlers) {
		struct vfs_request **iter = &req->backend->queue;
		while (*iter != NULL) // find free spot in queue
			iter = &(*iter)->queue_next;
		*iter = req;
		vfs_backend_tryaccept(req->backend);
	} else {
		vfsreq_finish_short(req, -1);
	}
}

void vfsreq_finish(struct vfs_request *req, char __user *stored, long ret,
		int flags, struct process *handler)
{
	if (req->type == VFSOP_OPEN && ret >= 0) {
		// TODO write tests for caller getting killed while opening a file
		if (!req->caller) panic_unimplemented();

		handle_t handle = process_find_free_handle(req->caller, 0);
		if (handle < 0)
			panic_invalid_state(); // we check for free handles before the open() call

		if (!(flags & FSR_DELEGATE)) {
			/* default behavior - create a new handle for the file, wrap the id */
			struct handle *backing = handle_init(HANDLE_FILE);
			backing->backend = req->backend;
			req->backend->refcount++;
			backing->file_id = stored;
			req->caller->handles[handle] = backing;
		} else {
			/* delegating - moving a handle to the caller */
			assert(handler);

			struct handle *h = handler->handles[ret];
			if (!h) {
				kprintf("tried delegating an invalid handle\n");
				handle = -1; // return error
			} else {
				req->caller->handles[handle] = h;
				handler->handles[ret] = NULL;
			}
		}
		ret = handle;
	}

	if (req->input.kern)
		kfree(req->input.buf_kern);

	if (req->backend)
		vfs_backend_refdown(req->backend);

	if (req->caller) {
		assert(req->caller->state == PS_WAITS4FS);
		regs_savereturn(&req->caller->regs, ret);
		process_transition(req->caller, PS_RUNNING);
	} else {
		kfree(req);
	}
}

void vfs_backend_tryaccept(struct vfs_backend *backend) {
	struct vfs_request *req = backend->queue;
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

void vfs_backend_user_accept(struct vfs_request *req) {
	struct process *handler;
	struct fs_wait_response res = {0};
	struct virt_cpy_error cpyerr;
	int len;

	assert(req && req->backend && req->backend->user.handler);
	handler = req->backend->user.handler;
	assert(handler->state == PS_WAITS4REQUEST);
	if (handler->handled_req)
		panic_unimplemented();

	// the virt_cpy calls aren't present in all kernel backends
	// it's a way to tell apart kernel and user backends apart
	// TODO check validity of memory regions somewhere else

	if (req->input.buf) {
		len = min(req->input.len, handler->awaited_req.max_len);
		virt_cpy(handler->pages, handler->awaited_req.buf,
				 req->input.kern ? NULL : req->caller->pages, req->input.buf,
				 len, &cpyerr);
		if (cpyerr.write_fail)
			panic_unimplemented();
		if (cpyerr.read_fail) {
			vfsreq_finish_short(req, -EFAULT);
			return;
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

	if (!virt_cpy_to(handler->pages,
				handler->awaited_req.res, &res, sizeof res))
	{
		panic_unimplemented();
	}

	process_transition(handler, PS_RUNNING);
	handler->handled_req = req;
	req->backend->user.handler = NULL;
	regs_savereturn(&handler->regs, 0);
	return;
}

void vfs_backend_refdown(struct vfs_backend *b) {
	assert(b);
	assert(b->refcount > 0);
	if (--(b->refcount) > 0) return;

	assert(!b->queue);
	if (b->heap) kfree(b);
}

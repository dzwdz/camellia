#include <kernel/arch/i386/driver/fsroot.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

void vfsreq_create(struct vfs_request req_) {
	struct vfs_request *req = kmalloc(sizeof *req); // freed in vfsreq_finish
	memcpy(req, &req_, sizeof *req);

	if (req->backend)
		req->backend->refcount++;

	if (req->caller) {
		process_transition(req->caller, PS_WAITS4FS);
		req->caller->waits4fs.req = req;
	}

	if (!req->backend || !req->backend->potential_handlers)
		vfsreq_finish(req, -1);

	struct vfs_request **iter = &req->backend->queue;
	while (*iter != NULL) // find free spot in queue
		iter = &(*iter)->queue_next;
	*iter = req;

	vfs_backend_tryaccept(req->backend);
}

void vfsreq_finish(struct vfs_request *req, int ret) {
	if (req->type == VFSOP_OPEN && ret >= 0) {
		// open() calls need special handling
		// we need to wrap the id returned by the VFS in a handle passed to
		// the client
		if (req->caller) {
			handle_t handle = process_find_free_handle(req->caller, 0);
			if (handle < 0)
				panic_invalid_state(); // we check for free handles before the open() call

			struct handle *backing = handle_init(HANDLE_FILE);
			backing->backend = req->backend;
			req->backend->refcount++;
			backing->file_id = ret;
			req->caller->handles[handle] = backing;
			ret = handle;
		} else {
			// caller got killed
			// TODO write tests & implement
			panic_unimplemented();
		}
	}

	if (req->input.kern)
		kfree(req->input.buf_kern);

	if (req->caller) {
		assert(req->caller->state == PS_WAITS4FS);
		regs_savereturn(&req->caller->regs, ret);
		process_transition(req->caller, PS_RUNNING);
	}

	vfs_backend_refdown(req->backend);
	kfree(req);
	return;
}

void vfs_backend_tryaccept(struct vfs_backend *backend) {
	struct vfs_request *req = backend->queue;
	if (!req) return;

	/* ensure backend is ready to accept request */
	if (backend->is_user) {
		if (!backend->user.handler) return;
	} else {
		assert(backend->kern.ready);
		if (!backend->kern.ready(backend)) return;
	}

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
	int len = 0;

	assert(req && req->backend && req->backend->user.handler);
	handler = req->backend->user.handler;
	assert(handler->state == PS_WAITS4REQUEST);
	assert(handler->handled_req == NULL);

	// the virt_cpy calls aren't present in all kernel backends
	// it's a way to tell apart kernel and user backends apart
	// TODO check validity of memory regions somewhere else

	if (req->input.buf) {
		len = min(req->input.len, handler->awaited_req.max_len);
		if (!virt_cpy(handler->pages, handler->awaited_req.buf, 
					req->input.kern ? NULL : req->caller->pages, req->input.buf, len))
			goto fail; // can't copy buffer
	}

	res.len      = len;
	res.capacity = req->output.len;
	res.id       = req->id;
	res.offset   = req->offset;
	res.op       = req->type;

	if (!virt_cpy_to(handler->pages,
				handler->awaited_req.res, &res, sizeof res))
		goto fail; // can't copy response struct

	process_transition(handler, PS_RUNNING);
	handler->handled_req = req;
	req->backend->user.handler = NULL;
	regs_savereturn(&handler->regs, 0);
	return;
fail:
	panic_unimplemented(); // TODO
}

void vfs_backend_refdown(struct vfs_backend *b) {
	assert(b);
	assert(b->refcount > 0);
	if (--(b->refcount) > 0) return;

	assert(!b->queue);
	if (b->heap) kfree(b);
}

#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <kernel/vfs/root.h>

int vfs_request_create(struct vfs_request req_) {
	struct vfs_request *req;
	process_transition(process_current, PS_WAITS4FS);
	process_current->waits4fs.queue_next = NULL;

	// the request is owned by the caller
	process_current->waits4fs.req = req_;
	req = &process_current->waits4fs.req;

	if (!req->backend || !req->backend->potential_handlers)
		return vfs_request_finish(req, -1);

	switch (req->backend->type) {
		case VFS_BACK_ROOT:
			return vfs_root_handler(req);
		case VFS_BACK_USER: {
			struct process **iter = &req->backend->queue;
			while (*iter != NULL) // find free spot in queue
				iter = &(*iter)->waits4fs.queue_next;
			*iter = req->caller;

			vfs_backend_accept(req->backend);
			return -1; // isn't passed to the caller process anyways
		}
		default:
			panic_invalid_state();
	}
}

int vfs_backend_accept(struct vfs_backend *backend) {
	struct vfs_request *req;
	struct process *handler = backend->handler;
	struct fs_wait_response res = {0};
	int len;

	if (!backend->handler) return -1;
	assert(handler->state == PS_WAITS4REQUEST);
	assert(!handler->handled_req);

	if (!backend->queue) return -1;
	// TODO wouldn't it be better to directly store vfs_requests in the queue?
	req = &backend->queue->waits4fs.req;
	backend->queue = backend->queue->waits4fs.queue_next;

	len = min(req->input.len, handler->awaited_req.max_len);
	if (!virt_cpy(handler->pages, handler->awaited_req.buf, 
				req->input.kern ? NULL : req->caller->pages, req->input.buf, len))
		goto fail; // can't copy buffer

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
	req->backend->handler = NULL;
	regs_savereturn(&handler->regs, 0);
	return 0;
fail:
	panic_unimplemented(); // TODO
}

int vfs_request_finish(struct vfs_request *req, int ret) {
	if (req->type == VFSOP_OPEN && ret >= 0) {
		// open() calls need special handling
		// we need to wrap the id returned by the VFS in a handle passed to
		// the client
		handle_t handle = process_find_handle(req->caller);
		if (handle < 0)
			panic_invalid_state(); // we check for free handles before the open() call
		req->caller->handles[handle] = (struct handle){
			.type = HANDLE_FILE,
			.file = {
				.backend = req->backend,
				.id = ret,
			},
		};
		ret = handle;
	}

	if (req->input.kern)
		kfree(req->input.buf_kern);

	assert(req->caller->state == PS_WAITS4FS || req->caller->state == PS_WAITS4IRQ);
	process_transition(req->caller, PS_RUNNING);
	regs_savereturn(&req->caller->regs, ret);
	return ret;
}

void vfs_request_cancel(struct vfs_request *req, int ret) {
	if (req->input.kern)
		kfree(req->input.buf_kern);

	// ret must always be negative, so it won't be confused with a success
	if (ret > 0) ret = -ret;
	if (ret == 0) ret = -1;
	regs_savereturn(&req->caller->regs, ret);
	process_transition(req->caller, PS_RUNNING);
}

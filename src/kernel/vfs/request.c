#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <kernel/vfs/root.h>

// dispatches a VFS operation to the correct process
int vfs_request_create(struct vfs_request req_) {
	struct vfs_request *req;
	int ret;
	process_current->state = PS_WAITS4FS;

	// the request is owned by the caller
	process_current->pending_req = req_;
	req = &process_current->pending_req;

	if (!req->backend)
		return vfs_request_finish(req, -1);

	switch (req->backend->type) {
		case VFS_BACK_ROOT:
			ret = vfs_root_handler(req);
			ret = vfs_request_finish(req, ret);
			return ret;
		case VFS_BACK_USER:
			if (req->backend->handler == NULL) {
				// backend isn't ready yet, join the queue
				assert(req->backend->queue == NULL); // TODO implement a proper queue
				req->backend->queue = process_current;
				process_switch_any();
			} else {
				vfs_request_pass2handler(req);
			}
		default:
			panic();
	}
}

_Noreturn void vfs_request_pass2handler(struct vfs_request *req) {
	struct process *handler = req->backend->handler;
	int len;
	assert(handler);
	assert(handler->state == PS_WAITS4REQUEST);
	assert(!handler->handled_req);
	handler->state = PS_RUNNING;
	handler->handled_req = req;

	if (!virt_cpy_from(handler->pages,
				&len, handler->awaited_req.len, sizeof len))
		goto fail; // can't read buffer length
	if (len > req->input.len) {
		// input bigger than buffer
		// TODO what should be done during e.g. open() calls? truncating doesn't seem right
		len = req->input.len;
	}

	if (req->input.kern) {
		if (!virt_cpy_to(handler->pages,
					handler->awaited_req.buf, req->input.buf_kern, len))
			goto fail; // can't copy buffer
	} else {
		if (!virt_cpy(handler->pages, handler->awaited_req.buf, 
					req->caller->pages, req->input.buf, len))
			goto fail; // can't copy buffer
	}

	if (!virt_cpy_to(handler->pages,
				handler->awaited_req.len, &len, sizeof len))
		goto fail; // can't copy new length

	if (!virt_cpy_to(handler->pages,
				handler->awaited_req.id, &req->id, sizeof req->id))
		goto fail; // can't copy id

	regs_savereturn(&handler->regs, req->type);
	process_switch(handler);
fail:
	panic(); // TODO
}

int vfs_request_finish(struct vfs_request *req, int ret) {
	struct process *caller = req->caller;

	if (req->type == VFSOP_OPEN && ret >= 0) {
		// open() calls need special handling
		// we need to wrap the id returned by the VFS in a handle passed to
		// the client
		handle_t handle = process_find_handle(req->caller);
		if (handle < 0) panic();
		req->caller->handles[handle] = (struct handle){
			.type = HANDLE_FILE,
			.file = {
				.backend = req->backend,
				.id = ret,
			},
		};
		ret = handle;
	}

	if (req->input.kern)  kfree(req->input.buf_kern);

	req->caller->state = PS_RUNNING;
	regs_savereturn(&req->caller->regs, ret);
	return ret;
}

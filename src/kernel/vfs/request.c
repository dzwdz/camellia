#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <kernel/vfs/root.h>
#include <shared/mem.h>

int vfsreq_create(struct vfs_request req_) {
	struct vfs_request *req = kmalloc(sizeof *req);
	memcpy(req, &req_, sizeof *req);
	/* freed in vfsreq_finish */

	if (req->backend)
		req->backend->refcount++;

	if (req->caller) {
		process_transition(req->caller, PS_WAITS4FS);
		req->caller->waits4fs.req = req;
	}

	if (!req->backend || !req->backend->potential_handlers)
		return vfsreq_finish(req, -1);

	switch (req->backend->type) {
		case VFS_BACK_ROOT:
			return vfs_root_handler(req);
		case VFS_BACK_USER: {
			struct vfs_request **iter = &req->backend->queue;
			while (*iter != NULL) // find free spot in queue
				iter = &(*iter)->queue_next;
			*iter = req;

			vfs_backend_accept(req->backend);
			return -1; // isn't passed to the caller process anyways
		}
		default:
			panic_invalid_state();
	}
}

int vfsreq_finish(struct vfs_request *req, int ret) {
	if (req->type == VFSOP_OPEN && ret >= 0) {
		// open() calls need special handling
		// we need to wrap the id returned by the VFS in a handle passed to
		// the client
		if (req->caller) {
			handle_t handle = process_find_handle(req->caller, 0);
			if (handle < 0)
				panic_invalid_state(); // we check for free handles before the open() call

			struct handle *backing = handle_init(HANDLE_FILE);
			backing->file.backend = req->backend;
			req->backend->refcount++;
			backing->file.id = ret;
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
		assert(req->caller->state == PS_WAITS4FS || req->caller->state == PS_WAITS4IRQ);
		regs_savereturn(&req->caller->regs, ret);
		process_transition(req->caller, PS_RUNNING);
	}

	vfs_backend_refdown(req->backend);
	kfree(req);
	return ret;
}

int vfs_backend_accept(struct vfs_backend *backend) {
	struct vfs_request *req = backend->queue;
	struct process *handler = backend->handler;
	struct fs_wait_response res = {0};
	int len = 0;

	if (!handler) return -1;
	assert(handler->state == PS_WAITS4REQUEST);
	assert(!handler->handled_req);

	if (!req) return -1;
	backend->queue = req->queue_next;

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
	req->backend->handler = NULL;
	regs_savereturn(&handler->regs, 0);
	return 0;
fail:
	panic_unimplemented(); // TODO
}

void vfs_backend_refdown(struct vfs_backend *b) {
	assert(b);
	assert(b->refcount > 0);
	if (--(b->refcount) > 0) return;

	assert(!b->queue);
	kfree(b);
}

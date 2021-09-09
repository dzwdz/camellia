#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/backend.h>
#include <kernel/vfs/root.h>

// dispatches a VFS operation to the correct process
_Noreturn void vfs_backend_dispatch(struct vfs_backend *backend, struct vfs_op op) {
	struct vfs_op_request *req = kmalloc(sizeof *req); // freed in vfs_backend_finish
	*req = (struct vfs_op_request) {
		.op = op,
		.caller = process_current,
		.backend = backend,
	};

	switch (backend->type) {
		case VFS_BACK_ROOT:
			int ret = vfs_root_handler(req);
			vfs_backend_finish(req, ret);
		case VFS_BACK_USER:
			process_current->state = PS_WAITS4FS;
			if (backend->handler == NULL) { 
				// backend isn't ready yet, join the queue
				assert(backend->queue == NULL); // TODO implement a proper queue

				backend->queue = process_current;
				process_current->pending_req = req;
				process_switch_any();
			} else {
				vfs_request_pass2handler(req);
			}
		default:
			panic();
	}
}

_Noreturn void vfs_request_pass2handler(struct vfs_op_request *req) {
	assert(req->backend->handler);
	assert(req->backend->handler->state == PS_WAITS4REQUEST);

	req->backend->handler->state = PS_RUNNING;
	// TODO pass the request to the process
	process_switch(req->backend->handler);
}

// returns from a VFS operation to the calling process
_Noreturn void vfs_backend_finish(struct vfs_op_request *req, int ret) {
	struct process *caller = req->caller;

	if (req->op.type == VFSOP_OPEN && ret >= 0) {
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

	if (req->op.type == VFSOP_OPEN)
		kfree(req->op.open.path);

	req->caller->state = PS_RUNNING;
	regs_savereturn(&req->caller->regs, ret);
	kfree(req);
	process_switch(caller);
}

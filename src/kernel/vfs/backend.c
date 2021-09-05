#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/backend.h>
#include <kernel/vfs/root.h>

// dispatches a VFS operation to the correct process
_Noreturn void vfs_backend_dispatch(struct vfs_backend *backend, struct vfs_op op) {
	struct vfs_op_request req = {
		.op = op,
		.caller = process_current,
		.backend = backend,
	};
	switch (backend->type) {
		case VFS_BACK_ROOT:
			int ret = vfs_root_handler(&req);
			vfs_backend_finish(&req, ret);
		default:
			panic();
	}
}

// returns from a VFS operation to the calling process
_Noreturn void vfs_backend_finish(struct vfs_op_request *req, int ret) {
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

	req->caller->state = PS_RUNNING;
	regs_savereturn(&req->caller->regs, ret);
	process_switch(req->caller);
}

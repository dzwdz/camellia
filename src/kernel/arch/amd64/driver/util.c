#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>

int req_readcopy(struct vfs_request *req, const void *buf, size_t len) {
	if (!req->caller) return -1;
	assert(req->type == VFSOP_READ);
	fs_normslice(&req->offset, &req->output.len, len, false);
	virt_cpy_to(
			req->caller->pages, req->output.buf,
			buf + req->offset, req->output.len);
	/* read errors are ignored. TODO write a spec */
	return req->output.len;
}

void postqueue_join(struct vfs_request **queue, struct vfs_request *req) {
	if (req->postqueue_next)
		panic_invalid_state();

	while (*queue)
		queue = &(*queue)->postqueue_next;
	*queue = req;
}

bool postqueue_pop(struct vfs_request **queue, void (*accept)(struct vfs_request *)) {
	struct vfs_request *req = *queue;
	if (req == NULL) return false;
	*queue = req->postqueue_next;
	accept(req);
	return true;
}

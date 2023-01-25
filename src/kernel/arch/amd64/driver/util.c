#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>

int req_readcopy(VfsReq *req, const void *buf, size_t len) {
	if (!req->caller) return -1;
	assert(req->type == VFSOP_READ);
	fs_normslice(&req->offset, &req->output.len, len, false);
	/* read errors are ignored. TODO write a spec */
	pcpy_to(req->caller, req->output.buf, buf + req->offset, req->output.len);
	return req->output.len;
}

void postqueue_join(VfsReq **queue, VfsReq *req) {
	if (req->postqueue_next)
		panic_invalid_state();

	while (*queue)
		queue = &(*queue)->postqueue_next;
	*queue = req;
}

bool postqueue_pop(VfsReq **queue, void (*accept)(VfsReq *)) {
	VfsReq *req = *queue;
	if (req == NULL) return false;
	*queue = req->postqueue_next;
	accept(req);
	return true;
}

void postqueue_ringreadall(VfsReq **queue, ring_t *r) {
	VfsReq *req;
	char tmp[64];
	size_t mlen = 0;
	if (ring_used(r) == 0) return;

	/* read as much as the biggest request wants */
	for (req = *queue; req; req = req->postqueue_next)
		mlen = max(mlen, req->output.len);
	mlen = min(mlen, sizeof tmp);
	mlen = ring_get(r, tmp, mlen);

	for (req = *queue; req; req = req->postqueue_next) {
		size_t ret = min(mlen, req->output.len);
		assert(req->type == VFSOP_READ);
		if (req->caller) {
			pcpy_to(req->caller, req->output.buf, tmp, ret);
		}
		vfsreq_finish_short(req, ret);
	}
	*queue = NULL;
}

size_t ring_to_virt(ring_t *r, Proc *proc, void __user *ubuf, size_t max) {
	char tmp[32];
	if (max > sizeof tmp) max = sizeof tmp;
	max = ring_get(r, tmp, max);
	return pcpy_to(proc, ubuf, tmp, max);
}

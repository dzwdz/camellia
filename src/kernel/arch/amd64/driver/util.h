#pragma once
#include <kernel/types.h>
#include <shared/ring.h>
#include <stdbool.h>
#include <stddef.h>

int req_readcopy(VfsReq *req, const void *buf, size_t len);

/* compare request path. path MUST be a static string */
#define reqpathcmp(req, path) _reqpathcmp(req, ""path"", sizeof(path) - 1)
#define _reqpathcmp(req, path, plen) \
	(req->input.kern && \
	 req->input.len == plen && \
	 memcmp(req->input.buf_kern, path, plen) == 0)

void postqueue_join(VfsReq **queue, VfsReq *req);
bool postqueue_pop(VfsReq **queue, void (*accept)(VfsReq *));

/** If there are any pending read requests, and the ring buffer isn't empty, fulfill them
 * all with a single read. */
void postqueue_ringreadall(VfsReq **queue, ring_t *r);

size_t ring_to_virt(ring_t *r, Proc *proc, void __user *ubuf, size_t max);

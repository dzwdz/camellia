#pragma once
#include <shared/container/ring.h>
#include <stdbool.h>
#include <stddef.h>

struct vfs_request;
int req_readcopy(struct vfs_request *req, const void *buf, size_t len);

/* compare request path. path MUST be a static string */
#define reqpathcmp(req, path) _reqpathcmp(req, ""path"", sizeof(path) - 1)
#define _reqpathcmp(req, path, plen) \
	(req->input.kern && \
	 req->input.len == plen && \
	 memcmp(req->input.buf_kern, path, plen) == 0)

void postqueue_join(struct vfs_request **queue, struct vfs_request *req);
bool postqueue_pop(struct vfs_request **queue, void (*accept)(struct vfs_request *));

/** If there are any pending read requests, and the ring buffer isn't empty, fulfill them
 * all with a single read. */
void postqueue_ringreadall(struct vfs_request **queue, ring_t *r);

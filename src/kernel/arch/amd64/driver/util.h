#pragma once
#include <stddef.h>

struct vfs_request;
int req_readcopy(struct vfs_request *req, const void *buf, size_t len);

/* compare request path. path MUST be a static string */
#define reqpathcmp(req, path) _reqpathcmp(req, ""path"", sizeof(path) - 1)
#define _reqpathcmp(req, path, plen) \
	(req->input.kern && \
	 req->input.len == plen && \
	 memcmp(req->input.buf_kern, path, plen) == 0)

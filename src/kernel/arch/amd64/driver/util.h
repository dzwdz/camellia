#pragma once
#include <stddef.h>

struct vfs_request;
int req_readcopy(struct vfs_request *req, const void *buf, size_t len);

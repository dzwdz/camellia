#pragma once
#include <camellia/types.h>

/* c0 - fs_wait returning a handle */
long c0_fs_wait(char *buf, long len, struct ufs_request *res);
long c0_fs_respond(void *buf, long ret, int flags);

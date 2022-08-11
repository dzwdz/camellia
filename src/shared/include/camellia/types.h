#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __CHECKER__
#  define __user  __attribute__((noderef, address_space(__user)))
#  define __force __attribute__((force))
#else
#  define __user
#  define __force
#endif

typedef void __user * userptr_t;
typedef int handle_t;

enum vfs_operation {
	VFSOP_OPEN,
	VFSOP_READ,
	VFSOP_WRITE,
	VFSOP_GETSIZE,
	VFSOP_REMOVE,
	VFSOP_CLOSE,
};

struct fs_wait_response {
	enum vfs_operation op;
	size_t len; // how much was put in *buf
	size_t capacity; // how much output can be accepted by the caller
	void __user *id;  // file id (returned by the open handler, passed to other calls)
	long offset;
	int flags;
};

#pragma once
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
	VFSOP_CLOSE,
};

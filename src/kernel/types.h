#pragma once
#include <stdint.h>

#define TYPES_INCLUDED

#ifdef __CHECKER__
#  define __user __attribute__((noderef, address_space(__user)))
#else
#  define __user
#endif

typedef void __user * userptr_t;

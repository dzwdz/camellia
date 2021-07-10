#pragma once
#include <arch/generic.h>

// dumb c shit
#define panic_tostr2(x) #x
#define panic_tostr(x)  panic_tostr2(x)

#define panic() do { \
	log_const(" PANIC! at the "); \
	log_const(__func__); \
	log_const(" (" __FILE__ ":" panic_tostr(__LINE__) ") "); \
	halt_cpu(); \
} while (0)

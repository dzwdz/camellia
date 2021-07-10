#pragma once
#include <arch/generic.h>
#include <arch/i386/tty.h> // TODO abstract away

// dumb c shit
#define panic_tostr2(x) #x
#define panic_tostr(x)  panic_tostr2(x)

#define panic() do { \
	tty_const(" PANIC! at the "); \
	tty_const(__func__); \
	tty_const(" (" __FILE__ ":" panic_tostr(__LINE__) ") "); \
	halt_cpu(); \
} while (0)

#pragma once
#include <kernel/arch/generic.h>
#include <kernel/util.h>

#define panic() do { \
	tty_const(" PANIC! at the "); \
	tty_const(__func__); \
	tty_const(" (" __FILE__ ":" NUM2STR(__LINE__) ") "); \
	halt_cpu(); \
} while (0)

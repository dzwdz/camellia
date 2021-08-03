#pragma once
#include <kernel/arch/generic.h>
#include <kernel/util.h>

#define panic() do { \
	log_const(" PANIC! at the "); \
	log_const(__func__); \
	log_const(" (" __FILE__ ":" NUM2STR(__LINE__) ") "); \
	halt_cpu(); \
} while (0)

#pragma once
#include <kernel/arch/generic.h>
#include <kernel/util.h>

#define _panic(type) do { \
	kprintf(" an "type" PANIC! at the "); \
	kprintf(__func__); \
	kprintf(" (" __FILE__ ":" NUM2STR(__LINE__) ") "); \
	halt_cpu(); \
} while (0)

#define panic_invalid_state() _panic("invalid state")
#define panic_unimplemented() _panic("unimplemented")
#define assert(stmt) do { if (!(stmt)) _panic("assert"); } while (0)

#undef panic

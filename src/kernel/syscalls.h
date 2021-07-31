// note: this file gets included in both kernel and userland
#pragma once
#include <stddef.h>

enum {
	// idc about stable syscall numbers just yet
	_SYSCALL_EXIT,
	_SYSCALL_FORK,

	_SYSCALL_DEBUGLOG
};

_Noreturn void _syscall_exit(const char *msg, size_t len);
int _syscall_fork();
int _syscall_debuglog(const char *msg, size_t len);

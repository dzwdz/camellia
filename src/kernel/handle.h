#pragma once
#include <kernel/types.h>
#include <stddef.h>

#define HANDLE_MAX 16

typedef int handle_t; // TODO duplicated in syscalls.h

enum handle_type {
	HANDLE_EMPTY,
};

struct handle {
	enum handle_type type;
};

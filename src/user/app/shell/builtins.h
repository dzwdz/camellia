#pragma once
#include <camellia/syscalls.h>
#include <stdbool.h>

struct builtin {
	const char *name;
	void (*fn)(int argc, const char **argv);
};

extern struct builtin builtins[];

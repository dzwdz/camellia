#pragma once
#include <camellia/syscalls.h>
#include <stdbool.h>

struct builtin {
	const char *name;
	void (*fn)(int argc, char **argv);
};

extern struct builtin builtins[];

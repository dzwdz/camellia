#pragma once
#include <stdbool.h>
#include <stddef.h>

struct redir {
	const char *stdout;
	bool append;
};

int parse(char *s, char **argv, size_t argvlen, struct redir *redir);
void run_args(int argc, char **argv, struct redir *redir);

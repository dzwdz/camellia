#pragma once
#include <bits/file.h>

struct execdata {
	int argc;
	char **argv, **envp;
	char *cwd;
};

void elf_execf(FILE *f, char **argv, char **envp);
void elf_exec(void *base, char **argv, char **envp);
void *elf_partialexec(void *elf); /* returns pointer to entry point */

void elf_selfreloc(void); // elfreloc.c

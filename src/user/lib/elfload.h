#pragma once
#include <bits/file.h>

void elf_execf(FILE *f);
void elf_exec(void *elf);
void *elf_partialexec(void *elf); /* returns pointer to entry point */

void elf_selfreloc(void); // elfreloc.c

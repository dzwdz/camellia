#pragma once
#include <user/lib/stdlib.h>

void elf_execf(libc_file *f);
void elf_exec(void *elf);

void elf_selfreloc(void); // elfreloc.c

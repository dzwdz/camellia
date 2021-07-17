#pragma once

#include <arch/log.h>

// i have no idea where else to put it
// some code assumes that it's a power of 2
#define PAGE_SIZE 4096

// src/arch/i386/boot.s
extern void stack_top;

__attribute__((noreturn))
void halt_cpu();

// src/arch/i386/sysenter.s
void sysexit(void (*fun)(), void *stack_top);
void sysenter_setup();

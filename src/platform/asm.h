#pragma once

// boot.s
__attribute__((noreturn))
void halt_cpu();

// sysenter.c
void sysexit(void (*fun)(), void *stack_top);
void sysenter_setup();

#pragma once

// sysenter.c
extern struct registers _sysexit_regs;
_Noreturn void sysenter_stage2(void);

// sysenter.s
void _sysexit_real(void);

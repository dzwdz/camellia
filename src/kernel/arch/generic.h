#pragma once
#include <kernel/arch/amd64/registers.h>
#include <kernel/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

// i have no idea where else to put it
// some code assumes that it's a power of 2
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)

// linker.ld
extern char _bss_end;

__attribute__((noreturn))
void cpu_halt(void);
__attribute__((noreturn))
void cpu_shutdown(void);
__attribute__((noreturn))
void shutdown(void);

/** on x86: waits for an IRQ */
void cpu_pause(void);

uint64_t uptime_ms(void);
void timer_schedule(Proc *p, uint64_t time);
void timer_deschedule(Proc *p);

// src/arch/i386/sysenter.s
_Noreturn void sysexit(CpuRegs);

// all of those can allocate memory
Pagedir *pagedir_new(void);
Pagedir *pagedir_copy(const Pagedir *orig);

void pagedir_free(Pagedir *);
void pagedir_unmap_user(Pagedir *dir, void __user *virt, size_t len);
void pagedir_map(Pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable);
bool pagedir_iskern(Pagedir *, const void __user *virt);

void __user *pagedir_findfree(Pagedir *dir, char __user *start, size_t len);

void pagedir_switch(Pagedir *);

// return 0 on failure
void *pagedir_virt2phys(Pagedir *dir, const void __user *virt,
                        bool user, bool writeable);

int kprintf(const char *fmt, ...);

void *debug_caller(size_t depth);
void debug_stacktrace(void);

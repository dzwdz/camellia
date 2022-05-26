#pragma once

#include <kernel/arch/i386/registers.h>
#include <shared/types.h>
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
void halt_cpu(void);
__attribute__((noreturn))
void cpu_shutdown(void);

/** on x86: waits for an IRQ */
void cpu_pause(void);

// src/arch/i386/sysenter.s
_Noreturn void sysexit(struct registers);

// all of those can allocate memory
struct pagedir *pagedir_new(void);
struct pagedir *pagedir_copy(const struct pagedir *orig);

void pagedir_free(struct pagedir *);
void *pagedir_unmap(struct pagedir *dir, void __user *virt);
void pagedir_map(struct pagedir *dir, void __user *virt, void *phys,
                 bool user, bool writeable);
bool pagedir_iskern(struct pagedir *, const void __user *virt);

void __user *pagedir_findfree(struct pagedir *dir, char __user *start, size_t len);

void pagedir_switch(struct pagedir *);

// return 0 on failure
void *pagedir_virt2phys(struct pagedir *dir, const void __user *virt,
                        bool user, bool writeable);

int kprintf(const char *fmt, ...);

void *debug_caller(size_t depth);
void debug_stacktrace(void);

#pragma once
#include <camellia/flags.h>
#include <sys/types.h>

#define MMAP_UNSUPPORTED 0xFFFF

#define PROT_EXEC 1
#define PROT_NONE MMAP_UNSUPPORTED
#define PROT_READ 1
#define PROT_WRITE 1

#define MAP_FIXED MMAP_UNSUPPORTED
#define MAP_PRIVATE 0
#define MAP_SHARED MMAP_UNSUPPORTED
#define MAP_ANONYMOUS 1

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);

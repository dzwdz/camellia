/* mmap() emulation */
#include <camellia/syscalls.h>
#include <errno.h>
#include <sys/mman.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
	(void)fd; (void) off;
	if ((flags & MMAP_UNSUPPORTED) == MMAP_UNSUPPORTED ||
		(prot & MMAP_UNSUPPORTED) == MMAP_UNSUPPORTED ||
		!(flags & MAP_ANONYMOUS))
	{
		errno = ENOSYS;
		return NULL;
	}

	void *p = _sys_memflag(addr, len, MEMFLAG_FINDFREE | MEMFLAG_PRESENT);
	if (!p) errno = ENOMEM;
	return p;
}

int munmap(void *addr, size_t len) {
	_sys_memflag(addr, len, 0);
	return 0;
}

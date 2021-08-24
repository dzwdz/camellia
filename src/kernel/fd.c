#include <kernel/fd.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

int fdop_dispatch(enum fdop fdop, fd_t fd, void *ptr, size_t len) {
	if (fd < 0 || fd >= FD_MAX) return -1;
	switch(process_current->fds[fd].type) {
		case FD_EMPTY:
			return -1;
		default:
			panic();
	}
}

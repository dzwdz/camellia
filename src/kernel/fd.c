#include <kernel/fd.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

int fdop_dispatch(enum fdop fdop, struct fd *fd, void *ptr, size_t len) {
	switch(fd->type) {
		case FD_EMPTY:
			return -1;
		default:
			panic();
	}
}

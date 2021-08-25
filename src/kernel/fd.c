#include <kernel/fd.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

static int fdop_special_tty(struct fdop_args *args);

int fdop_dispatch(struct fdop_args args) {
	switch(args.fd->type) {
		case FD_EMPTY: {
			if (args.type == FDOP_MOUNT) // mounting an empty fd is allowed
				return 0;
			return -1;
		}
		case FD_SPECIAL_TTY:
			return fdop_special_tty(&args);
		default:
			panic();
	}
}

static int fdop_special_tty(struct fdop_args *args) {
	switch(args->type) {
		case FDOP_MOUNT:
			return 0; // no special action needed

		case FDOP_READ:
			return -1; // input not implemented yet

		case FDOP_WRITE: {
			 struct virt_iter iter;
			 virt_iter_new(&iter, args->rw.ptr, args->rw.len,
					 process_current->pages, true, false);
			 while (virt_iter_next(&iter))
				 tty_write(iter.frag, iter.frag_len);
			 return iter.prior;
		}

		case FDOP_CLOSE:
			args->fd->type = FD_EMPTY;
			return 0;

		default: panic();
	}
}

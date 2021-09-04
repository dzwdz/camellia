#include <kernel/handle.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>

static int handleop_special_tty(struct handleop_args *args);

int handleop_dispatch(struct handleop_args args) {
	switch(args.handle->type) {
		case HANDLE_EMPTY: {
			if (args.type == HANDLEOP_MOUNT) // mounting an empty handle is allowed
				return 0;
			return -1;
		}
		case HANDLE_SPECIAL_TTY:
			return handleop_special_tty(&args);
		default:
			panic();
	}
}

static int handleop_special_tty(struct handleop_args *args) {
	switch(args->type) {
		case HANDLEOP_MOUNT:
			return 0; // no special action needed

		case HANDLEOP_OPEN:
			/* don't allow anything after the mount point
			 * this is a file, not a directory
			 * for example: open("/tty") is allowed. open("/tty/smth") isn't */
			if (args->open.len == 0) {
				args->open.target->type = HANDLE_SPECIAL_TTY;
				return 0;
			}
			return -1;

		case HANDLEOP_READ:
			return -1; // input not implemented yet

		case HANDLEOP_WRITE: {
			 struct virt_iter iter;
			 virt_iter_new(&iter, args->rw.ptr, args->rw.len,
					 process_current->pages, true, false);
			 while (virt_iter_next(&iter))
				 tty_write(iter.frag, iter.frag_len);
			 return iter.prior;
		}

		case HANDLEOP_CLOSE:
			args->handle->type = HANDLE_EMPTY;
			return 0;

		default: panic();
	}
}

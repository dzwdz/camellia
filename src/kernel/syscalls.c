#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/proc.h>
#include <kernel/vfs/path.h>
#include <shared/flags.h>
#include <shared/mem.h>
#include <shared/syscalls.h>
#include <stdint.h>

#define SYSCALL_RETURN(val) do { \
	uint32_t ret = (uint32_t)val; \
	assert(process_current->state == PS_RUNNING); \
	regs_savereturn(&process_current->regs, ret); \
	return 0; \
} while (0)

_Noreturn void _syscall_exit(int ret) {
	process_kill(process_current, ret);
	process_switch_any();
}

int _syscall_await(void) {
	bool has_children = false;
	process_transition(process_current, PS_WAITS4CHILDDEATH);

	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling)
	{
		if (iter->noreap) continue;
		has_children = true;
		if (iter->state == PS_DEAD) // TODO this path used to crash, still untested
			SYSCALL_RETURN(process_try2collect(iter));
	}

	if (!has_children) {
		process_transition(process_current, PS_RUNNING);
		SYSCALL_RETURN(~0); // TODO errno
	}

	return -1; // dummy
}

int _syscall_fork(int flags, handle_t __user *fs_front) {
	struct process *child;
	handle_t front;

	if ((flags & FORK_NEWFS) && fs_front) {
		/* we'll need to return a handle, check if that's possible */
		front = process_find_free_handle(process_current, 1);
		if (front < 0) SYSCALL_RETURN(-1);
	}

	child = process_fork(process_current, flags);
	regs_savereturn(&child->regs, 0);

	if ((flags & FORK_NEWFS) && fs_front) {
		struct vfs_backend *backend = kmalloc(sizeof *backend);

		process_current->handles[front] = handle_init(HANDLE_FS_FRONT);

		backend->heap = true;
		backend->is_user = true;
		backend->potential_handlers = 1;
		backend->refcount = 2; // child + handle
		backend->user.handler = NULL;
		backend->queue = NULL;

		child->controlled = backend;

		process_current->handles[front]->backend = backend;

		if (fs_front) {
			/* failure ignored. if you pass an invalid pointer to this function,
			 * you just don't receive the handle. you'll probably segfault
			 * trying to access it anyways */
			virt_cpy_to(process_current->pages, fs_front, &front, sizeof front);
		}
	}
	SYSCALL_RETURN(1);
}

handle_t _syscall_open(const char __user *path, int len, int flags) {
	struct vfs_mount *mount;
	char *path_buf = NULL;

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);
	if (process_find_free_handle(process_current, 0) < 0)
		SYSCALL_RETURN(-1);

	path_buf = kmalloc(len);
	if (!path_buf) goto fail;
	if (!virt_cpy_from(process_current->pages, path_buf, path, len)) goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;

	mount = vfs_mount_resolve(process_current->mount, path_buf, len);
	if (!mount) goto fail;

	if (mount->prefix_len > 0) { // strip prefix
		len -= mount->prefix_len;
		// i can't just adjust path_buf, because it needs to be passed to free()
		// later on
		memcpy(path_buf, path_buf + mount->prefix_len, len);
	}

	vfsreq_create((struct vfs_request) {
			.type = VFSOP_OPEN,
			.input = {
				.kern = true,
				.buf_kern = path_buf,
				.len = len,
			},
			.caller = process_current,
			.backend = mount->backend,
			.flags = flags,
		});
	return -1; // dummy
fail:
	kfree(path_buf);
	SYSCALL_RETURN(-1);
}

int _syscall_mount(handle_t hid, const char __user *path, int len) {
	struct vfs_mount *mount = NULL;
	struct vfs_backend *backend = NULL;
	char *path_buf = NULL;

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);

	path_buf = kmalloc(len);
	if (!path_buf) goto fail;
	if (!virt_cpy_from(process_current->pages, path_buf, path, len)) goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;

	// remove trailing slash
	// mounting something under `/this` and `/this/` should be equalivent
	if (path_buf[len - 1] == '/') {
		if (len == 0) goto fail;
		len--;
	}

	if (hid >= 0) { // mounting a real backend?
		struct handle *handle = process_handle_get(process_current, hid);
		if (!handle || handle->type != HANDLE_FS_FRONT)
			goto fail;
		backend = handle->backend;
		backend->refcount++;
	} // otherwise backend == NULL

	// append to mount list
	// TODO move to kernel/vfs/mount.c
	mount = kmalloc(sizeof *mount);
	mount->prev = process_current->mount;
	mount->prefix = path_buf;
	mount->prefix_owned = true;
	mount->prefix_len = len;
	mount->backend = backend;
	mount->refs = 1;
	process_current->mount = mount;

	kmalloc_sanity(mount);
	kmalloc_sanity(mount->prefix);
	SYSCALL_RETURN(0);

fail:
	kfree(path_buf);
	kfree(mount);
	SYSCALL_RETURN(-1);
}

handle_t _syscall_dup(handle_t from, handle_t to, int flags) {
	struct handle *fromh, **toh;
	if (flags) SYSCALL_RETURN(-1);

	if (to < 0) {
		to = process_find_free_handle(process_current, 0);
		if (to < 0) SYSCALL_RETURN(-1);
	} else if (to >= HANDLE_MAX) {
		SYSCALL_RETURN(-1);
	}

	if (to == from)
		SYSCALL_RETURN(to);
	toh = &process_current->handles[to];
	fromh = (from >= 0 && from < HANDLE_MAX) ?
		process_current->handles[from] : NULL;

	if (*toh) handle_close(*toh);
	*toh = fromh;
	if (fromh) fromh->refcount++;

	SYSCALL_RETURN(to);
}

int _syscall_read(handle_t handle_num, void __user *buf, size_t len, int offset) {
	struct handle *h = process_handle_get(process_current, handle_num);
	if (!h) SYSCALL_RETURN(-1);
	switch (h->type) {
		case HANDLE_FILE:
			vfsreq_create((struct vfs_request) {
					.type = VFSOP_READ,
					.output = {
						.buf = (userptr_t) buf,
						.len = len,
					},
					.id = h->file_id,
					.offset = offset,
					.caller = process_current,
					.backend = h->backend,
				});
			break;
		case HANDLE_PIPE:
			if (pipe_joinqueue(h, true, process_current, buf, len))
				pipe_trytransfer(h);
			else
				SYSCALL_RETURN(-1);
			break;
		default:
			SYSCALL_RETURN(-1);
	}
	return -1; // dummy
}

int _syscall_write(handle_t handle_num, const void __user *buf, size_t len, int offset) {
	struct handle *h = process_handle_get(process_current, handle_num);
	if (!h) SYSCALL_RETURN(-1);
	switch (h->type) {
		case HANDLE_FILE:
			vfsreq_create((struct vfs_request) {
					.type = VFSOP_WRITE,
					.input = {
						.buf = (userptr_t) buf,
						.len = len,
					},
					.id = h->file_id,
					.offset = offset,
					.caller = process_current,
					.backend = h->backend,
				});
			break;
		case HANDLE_PIPE:
			if (pipe_joinqueue(h, false, process_current, (void __user *)buf, len))
				pipe_trytransfer(h);
			else
				SYSCALL_RETURN(-1);
			break;
		default:
			SYSCALL_RETURN(-1);
	}
	return -1; // dummy
}

int _syscall_close(handle_t hid) {
	if (hid < 0 || hid >= HANDLE_MAX) return -1;
	struct handle **h = &process_current->handles[hid];
	if (!*h) SYSCALL_RETURN(-1);
	handle_close(*h);
	*h = NULL;
	SYSCALL_RETURN(0);
}

int _syscall_fs_wait(char __user *buf, int max_len, struct fs_wait_response __user *res) {
	struct vfs_backend *backend = process_current->controlled;
	if (!backend) SYSCALL_RETURN(-1);

	process_transition(process_current, PS_WAITS4REQUEST);
	assert(!backend->user.handler); // TODO allow multiple processes to wait on the same backend
	backend->user.handler = process_current;
	/* checking the validity of those pointers here would make
	 * vfs_backend_accept simpler. TODO? */
	process_current->awaited_req.buf     = buf;
	process_current->awaited_req.max_len = max_len;
	process_current->awaited_req.res     = res;

	vfs_backend_tryaccept(backend); // sets return value
	return -1; // dummy
}

int _syscall_fs_respond(void __user *buf, int ret, int flags) {
	struct vfs_request *req = process_current->handled_req;
	if (!req) SYSCALL_RETURN(-1);

	if (req->output.len > 0 && ret > 0) {
		// if this vfsop outputs data and ret is positive, it's the length of the buffer
		// TODO document
		ret = min(ret, capped_cast32(req->output.len));
		if (!virt_cpy(req->caller->pages, req->output.buf,
					process_current->pages, buf, ret)) {
			// how should this error even be handled? TODO
		}
	}

	process_current->handled_req = NULL;
	vfsreq_finish(req, buf, ret, flags, process_current);
	SYSCALL_RETURN(0);
}

void __user *_syscall_memflag(void __user *addr, size_t len, int flags) {
	struct pagedir *pages = process_current->pages;
	void *phys;
	addr = (userptr_t)((int __force)addr & ~PAGE_MASK); // align to page boundary

	if (flags & MEMFLAG_FINDFREE) {
		addr = pagedir_findfree(pages, addr, len);
		if (!(flags & MEMFLAG_PRESENT))
			SYSCALL_RETURN((uintptr_t)addr);
	}


	for (userptr_t iter = addr; iter < addr + len; iter += PAGE_SIZE) {
		if (pagedir_iskern(pages, iter)) {
			// TODO reflect failure in return value
			continue;
		}

		phys = pagedir_virt2phys(pages, iter, false, false); 

		if (!(flags & MEMFLAG_PRESENT)) {
			if (phys)
				page_free(pagedir_unmap(pages, iter), 1);
			continue;
		}

		if (!phys) {
			phys = page_alloc(1);
			memset(phys, 0, PAGE_SIZE); // TODO somehow test this
			pagedir_map(pages, iter, phys, true, true);
		}
	}
	SYSCALL_RETURN((uintptr_t)addr);
}

int _syscall_pipe(handle_t __user user_ends[2], int flags) {
	if (flags) SYSCALL_RETURN(-1);
	handle_t ends[2];
	struct handle *rend, *wend;

	ends[0] = process_find_free_handle(process_current, 0);
	if (ends[0] < 0) SYSCALL_RETURN(-1);
	ends[1] = process_find_free_handle(process_current, ends[0]+1);
	if (ends[1] < 0) SYSCALL_RETURN(-1);

	rend = process_current->handles[ends[0]] = handle_init(HANDLE_PIPE);
	wend = process_current->handles[ends[1]] = handle_init(HANDLE_PIPE);
	wend->pipe.write_end = true;
	wend->pipe.sister = rend;
	rend->pipe.sister = wend;

	virt_cpy_to(process_current->pages, user_ends, ends, sizeof ends);
	SYSCALL_RETURN(0);
}

void _syscall_debug_klog(const void __user *buf, size_t len) {
	(void)buf; (void)len;
	// static char kbuf[256];
	// if (len >= sizeof(kbuf)) len = sizeof(kbuf) - 1;
	// virt_cpy_from(process_current->pages, kbuf, buf, len);
	// kbuf[len] = '\0';
	// kprintf("[klog] %x\t%s\n", process_current->id, kbuf);
}

int _syscall(int num, int a, int b, int c, int d) {
	switch (num) {
		case _SYSCALL_EXIT:
			_syscall_exit(a);
			// _syscall_exit doesn't exit
		case _SYSCALL_AWAIT:
			_syscall_await();
			break;
		case _SYSCALL_FORK:
			_syscall_fork(a, (userptr_t)b);
			break;
		case _SYSCALL_OPEN:
			_syscall_open((userptr_t)a, b, c);
			break;
		case _SYSCALL_MOUNT:
			_syscall_mount(a, (userptr_t)b, c);
			break;
		case _SYSCALL_DUP:
			_syscall_dup(a, b, c);
			break;
		case _SYSCALL_READ:
			_syscall_read(a, (userptr_t)b, c, d);
			break;
		case _SYSCALL_WRITE:
			_syscall_write(a, (userptr_t)b, c, d);
			break;
		case _SYSCALL_CLOSE:
			_syscall_close(a);
			break;
		case _SYSCALL_FS_WAIT:
			_syscall_fs_wait((userptr_t)a, b, (userptr_t)c);
			break;
		case _SYSCALL_FS_RESPOND:
			_syscall_fs_respond((userptr_t)a, b, c);
			break;
		case _SYSCALL_MEMFLAG:
			_syscall_memflag((userptr_t)a, b, c);
			break;
		case _SYSCALL_PIPE:
			_syscall_pipe((userptr_t)a, b);
			break;
		case _SYSCALL_DEBUG_KLOG:
			_syscall_debug_klog((userptr_t)a, b);
			break;
		default:
			regs_savereturn(&process_current->regs, -1);
			break;
	}

	/* return value is unused. execution continues in sysenter_stage2 */
	return -1;
}

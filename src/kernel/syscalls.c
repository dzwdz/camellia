#include <camellia/path.h>
#include <camellia/errno.h>
#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/proc.h>
#include <shared/mem.h>
#include <stdint.h>

#define SYSCALL_RETURN(val) do { \
	assert(process_current->state == PS_RUNNING); \
	regs_savereturn(&process_current->regs, (long)(val)); \
	return 0; \
} while (0)

_Noreturn void _syscall_exit(long ret) {
	process_kill(process_current, ret);
	process_switch_any();
}

long _syscall_await(void) {
	bool has_children = false;
	process_transition(process_current, PS_WAITS4CHILDDEATH);

	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling)
	{
		if (iter->noreap) continue;
		has_children = true;
		if (iter->state == PS_DEAD) {
			process_try2collect(iter);
			return 0; // dummy
		}
	}

	if (!has_children) {
		process_transition(process_current, PS_RUNNING);
		SYSCALL_RETURN(~0); // TODO errno
	}
	return 0; // dummy
}

long _syscall_fork(int flags, handle_t __user *fs_front) {
	struct process *child;

	child = process_fork(process_current, flags);
	regs_savereturn(&child->regs, 0);

	if ((flags & FORK_NEWFS) && fs_front) {
		struct handle *h;
		handle_t hid = process_handle_init(process_current, HANDLE_FS_FRONT, &h);
		if (hid < 0) {
			// TODO test
			child->noreap = true;
			process_kill(child, -EMFILE);
			SYSCALL_RETURN(-EMFILE);
		}

		h->backend = kmalloc(sizeof *h->backend);
		h->backend->is_user = true;
		h->backend->potential_handlers = 1;
		h->backend->refcount = 2; // child + handle
		h->backend->user.handler = NULL;
		h->backend->queue = NULL;
		child->controlled = h->backend;

		if (fs_front) {
			/* failure ignored. if you pass an invalid pointer to this function,
			 * you just don't receive the handle. you'll probably segfault
			 * trying to access it anyways */
			virt_cpy_to(process_current->pages, fs_front, &hid, sizeof hid);
		}
	}
	SYSCALL_RETURN(1);
}

handle_t _syscall_open(const char __user *path, long len, int flags) {
	struct vfs_mount *mount;
	char *path_buf = NULL;

	if (flags & ~(OPEN_CREATE | OPEN_RO)) SYSCALL_RETURN(-ENOSYS);

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);
	if (process_find_free_handle(process_current, 0) < 0)
		SYSCALL_RETURN(-1);

	path_buf = kmalloc(len);
	if (!path_buf) goto fail;
	if (!virt_cpy_from(process_current->pages, path_buf, path, len)) goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len == 0) goto fail;

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

long _syscall_mount(handle_t hid, const char __user *path, long len) {
	struct vfs_mount *mount = NULL;
	struct vfs_backend *backend = NULL;
	char *path_buf = NULL;

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);

	path_buf = kmalloc(len);
	if (!path_buf) goto fail;
	if (!virt_cpy_from(process_current->pages, path_buf, path, len)) goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len == 0) goto fail;

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
	if (flags != 0) SYSCALL_RETURN(-ENOSYS);
	SYSCALL_RETURN(process_handle_dup(process_current, from, to));
}

static long simple_vfsop(
		enum vfs_operation vfsop, handle_t hid, void __user *buf,
		size_t len, long offset, int flags)
{
	assert(vfsop != VFSOP_OPEN && vfsop != VFSOP_CLOSE);
	struct handle *h = process_handle_get(process_current, hid);
	if (!h) SYSCALL_RETURN(-1);
	if (h->type == HANDLE_FILE) {
		if (h->ro && !(vfsop == VFSOP_READ || vfsop == VFSOP_GETSIZE))
			SYSCALL_RETURN(-EACCES);
		struct vfs_request req = (struct vfs_request){
			.type = vfsop,
			.backend = h->backend,
			.id = h->file_id,
			.caller = process_current,
			.offset = offset,
			.flags = flags,
		};
		if (vfsop == VFSOP_READ) {
			req.output.buf = buf;
			req.output.len = len;
		}
		if (vfsop == VFSOP_WRITE) {
			req.input.buf = buf;
			req.input.len = len;
		}
		vfsreq_create(req);
	} else if (h->type == HANDLE_PIPE) {
		if (vfsop == VFSOP_READ || vfsop == VFSOP_WRITE) {
			pipe_joinqueue(h, vfsop == VFSOP_READ, process_current, buf, len);
		} else SYSCALL_RETURN(-ENOSYS);
	} else SYSCALL_RETURN(-ENOSYS);
	return 0;
}

long _syscall_read(handle_t hid, void __user *buf, size_t len, long offset) {
	simple_vfsop(VFSOP_READ, hid, buf, len, offset, 0);
	return 0;
}

long _syscall_write(handle_t hid, const void __user *buf, size_t len, long offset, int flags) {
	if (flags & ~(WRITE_TRUNCATE))
		SYSCALL_RETURN(-ENOSYS);
	simple_vfsop(VFSOP_WRITE, hid, (userptr_t)buf, len, offset, flags);
	return 0;
}

long _syscall_getsize(handle_t hid) {
	simple_vfsop(VFSOP_GETSIZE, hid, NULL, 0, 0, 0);
	return 0;
}

long _syscall_remove(handle_t hid) {
	struct handle *h = process_handle_get(process_current, hid);
	if (!h) SYSCALL_RETURN(-EBADF);
	if (!h->ro && h->type == HANDLE_FILE) {
		vfsreq_create((struct vfs_request) {
				.type = VFSOP_REMOVE,
				.id = h->file_id,
				.caller = process_current,
				.backend = h->backend,
			});
		process_handle_close(process_current, hid);
		return -1; // dummy
	} else {
		process_handle_close(process_current, hid);
		SYSCALL_RETURN(h->ro ? -EACCES : -ENOSYS);
	}
}

long _syscall_close(handle_t hid) {
	if (!process_handle_get(process_current, hid)) {
		SYSCALL_RETURN(-EBADF);
	}
	process_handle_close(process_current, hid);
	SYSCALL_RETURN(0);
}

handle_t _syscall_fs_wait(char __user *buf, long max_len, struct fs_wait_response __user *res) {
	struct vfs_backend *backend = process_current->controlled;
	// TODO can be used to tell if you're init
	if (!backend) SYSCALL_RETURN(-1);

	process_transition(process_current, PS_WAITS4REQUEST);
	if (backend->user.handler)
		panic_unimplemented();
	backend->user.handler = process_current;
	process_current->awaited_req.buf     = buf;
	process_current->awaited_req.max_len = max_len;
	process_current->awaited_req.res     = res;

	vfs_backend_tryaccept(backend); // sets return value
	return -1; // dummy
}

long _syscall_fs_respond(handle_t hid, const void __user *buf, long ret, int flags) {
	struct handle *h = process_handle_get(process_current, hid);
	if (!h || h->type != HANDLE_FS_REQ) SYSCALL_RETURN(-EBADF);
	struct vfs_request *req = h->req;
	if (req) {
		if (req->output.len > 0 && ret > 0) {
			// if this vfsop outputs data and ret is positive, it's the length of the buffer
			// TODO document
			// TODO move to vfsreq_finish
			ret = min(ret, capped_cast32(req->output.len));
			struct virt_cpy_error err;
			virt_cpy(req->caller->pages, req->output.buf,
					process_current->pages, buf, ret, &err);

			if (err.read_fail)
				panic_unimplemented();
			/* write failures are ignored */
		}
		vfsreq_finish(req, (void __user *)buf, ret, flags, process_current);
	}
	h->req = NULL;
	process_handle_close(process_current, hid);
	SYSCALL_RETURN(0);
}

void __user *_syscall_memflag(void __user *addr, size_t len, int flags) {
	struct pagedir *pages = process_current->pages;
	void *phys;
	addr = (userptr_t)((uintptr_t __force)addr & ~PAGE_MASK); // align to page boundary

	if (flags & MEMFLAG_FINDFREE) {
		addr = pagedir_findfree(pages, addr, len);
		if (!(flags & MEMFLAG_PRESENT))
			SYSCALL_RETURN((uintptr_t)addr);
	}

	if (!(flags & MEMFLAG_PRESENT)) {
		pagedir_unmap_user(pages, addr, len);
		SYSCALL_RETURN((uintptr_t)addr);
	}


	for (size_t off = 0; off < len; off += PAGE_SIZE) {
		userptr_t page = addr + off;
		if (pagedir_iskern(pages, page)) {
			// TODO reflect failure in return value
			continue;
		}

		phys = pagedir_virt2phys(pages, page, false, false);
		if (!phys) {
			// TODO test zeroing of user pages
			phys = page_zalloc(1);
			pagedir_map(pages, page, phys, true, true);
		}
	}
	SYSCALL_RETURN((uintptr_t)addr);
}

long _syscall_pipe(handle_t __user user_ends[2], int flags) {
	if (flags) SYSCALL_RETURN(-ENOSYS);

	handle_t ends[2];
	struct handle *rend, *wend;
	ends[0] = process_handle_init(process_current, HANDLE_PIPE, &rend);
	ends[1] = process_handle_init(process_current, HANDLE_PIPE, &wend);
	if (ends[0] < 0 || ends[1] < 0) {
		process_handle_close(process_current, ends[0]);
		process_handle_close(process_current, ends[1]);
		SYSCALL_RETURN(-EMFILE);
	}
	wend->pipe.write_end = true;
	wend->pipe.sister = rend;
	rend->pipe.sister = wend;

	virt_cpy_to(process_current->pages, user_ends, ends, sizeof ends);
	SYSCALL_RETURN(0);
}

void _syscall_sleep(long ms) {
	// TODO no overflow check - can leak current uptime
	timer_schedule(process_current, uptime_ms() + ms);
}

long _syscall_execbuf(void __user *ubuf, size_t len) {
	if (len == 0) SYSCALL_RETURN(0);
	if (len > EXECBUF_MAX_LEN)
		SYSCALL_RETURN(-1);
	if (process_current->execbuf.buf)
		SYSCALL_RETURN(-1);
	// TODO consider supporting nesting execbufs

	char *kbuf = kmalloc(len);
	if (!virt_cpy_from(process_current->pages, kbuf, ubuf, len)) {
		kfree(kbuf);
		SYSCALL_RETURN(-1);
	}
	process_current->execbuf.buf = kbuf;
	process_current->execbuf.len = len;
	process_current->execbuf.pos = 0;
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

long _syscall(long num, long a, long b, long c, long d, long e) {
	/* note: this isn't the only place where syscalls get called from!
	 *       see execbuf */
	switch (num) {
		break; case _SYSCALL_EXIT:	_syscall_exit(a);
		break; case _SYSCALL_AWAIT:	_syscall_await();
		break; case _SYSCALL_FORK:	_syscall_fork(a, (userptr_t)b);
		break; case _SYSCALL_OPEN:	_syscall_open((userptr_t)a, b, c);
		break; case _SYSCALL_MOUNT:	_syscall_mount(a, (userptr_t)b, c);
		break; case _SYSCALL_DUP:	_syscall_dup(a, b, c);
		break; case _SYSCALL_READ:	_syscall_read(a, (userptr_t)b, c, d);
		break; case _SYSCALL_WRITE:	_syscall_write(a, (userptr_t)b, c, d, e);
		break; case _SYSCALL_GETSIZE:	_syscall_getsize(a);
		break; case _SYSCALL_REMOVE:	_syscall_remove(a);
		break; case _SYSCALL_CLOSE:	_syscall_close(a);
		break; case _SYSCALL_FS_WAIT:	_syscall_fs_wait((userptr_t)a, b, (userptr_t)c);
		break; case _SYSCALL_FS_RESPOND:	_syscall_fs_respond(a, (userptr_t)b, c, d);
		break; case _SYSCALL_MEMFLAG:	_syscall_memflag((userptr_t)a, b, c);
		break; case _SYSCALL_PIPE:	_syscall_pipe((userptr_t)a, b);
		break; case _SYSCALL_SLEEP:	_syscall_sleep(a);
		break; case _SYSCALL_EXECBUF:	_syscall_execbuf((userptr_t)a, b);
		break; case _SYSCALL_DEBUG_KLOG:	_syscall_debug_klog((userptr_t)a, b);
		break;
		default:
			regs_savereturn(&process_current->regs, -1);
			break;
	}
	/* return value is unused. execution continues in sysenter_stage2 */
	return -1;
}

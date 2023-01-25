#include <camellia/path.h>
#include <camellia/errno.h>
#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/panic.h>
#include <kernel/pipe.h>
#include <kernel/proc.h>
#include <shared/mem.h>
#include <stdint.h>

#define SYSCALL_RETURN(val) do { \
	assert(proc_cur->state == PS_RUNNING); \
	regs_savereturn(&proc_cur->regs, (long)(val)); \
	return 0; \
} while (0)

_Noreturn void _sys_exit(long ret) {
	proc_kill(proc_cur, ret);
	proc_switch_any();
}

long _sys_await(void) {
	bool has_children = false;
	proc_setstate(proc_cur, PS_WAITS4CHILDDEATH);

	for (Proc *iter = proc_cur->child;
			iter; iter = iter->sibling)
	{
		if (iter->noreap) continue;
		has_children = true;
		if (iter->state == PS_TOREAP) {
			proc_tryreap(iter);
			return 0; // dummy
		}
	}

	if (!has_children) {
		proc_setstate(proc_cur, PS_RUNNING);
		SYSCALL_RETURN(~0); // TODO errno
	}
	return 0; // dummy
}

long _sys_fork(int flags, hid_t __user *fs_front) {
	Proc *child;

	child = proc_fork(proc_cur, flags);
	regs_savereturn(&child->regs, 0);

	if (flags & FORK_NEWFS) {
		Handle *h;
		hid_t hid = proc_handle_init(proc_cur, HANDLE_FS_FRONT, &h);
		if (hid < 0) {
			child->noreap = true;
			proc_kill(child, -EMFILE);
			SYSCALL_RETURN(-EMFILE);
		}

		h->backend = kzalloc(sizeof *h->backend);
		h->backend->is_user = true;
		h->backend->provhcnt = 1; /* child */
		h->backend->usehcnt = 1; /* handle */
		h->backend->user.handler = NULL;
		h->backend->queue = NULL;
		child->controlled = h->backend;

		if (fs_front) {
			/* failure ignored. if you pass an invalid pointer to this function,
			 * you just don't receive the handle. you'll probably segfault
			 * trying to access it anyways */
			pcpy_to(proc_cur, fs_front, &hid, sizeof hid);
		}
	}
	SYSCALL_RETURN(child->cid);
}

hid_t _sys_open(const char __user *path, long len, int flags) {
	VfsMount *mount;
	char *path_buf = NULL;

	if (flags & ~(OPEN_RW | OPEN_CREATE)) SYSCALL_RETURN(-ENOSYS);

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);
	/* Doesn't check for free handles. Another thread could use up all
	 * handles in the meantime anyways, or free some up. */

	path_buf = kmalloc(len);
	if (pcpy_from(proc_cur, path_buf, path, len) < (size_t)len) {
		goto fail;
	}

	len = path_simplify(path_buf, path_buf, len);
	if (len == 0) goto fail;

	mount = vfs_mount_resolve(proc_cur->mount, path_buf, len);
	if (!mount) goto fail;

	if (mount->prefix_len > 0) { // strip prefix
		len -= mount->prefix_len;
		// i can't just adjust path_buf, because it needs to be passed to free()
		// later on
		memcpy(path_buf, path_buf + mount->prefix_len, len);
	}

	vfsreq_create((VfsReq) {
			.type = VFSOP_OPEN,
			.input = {
				.kern = true,
				.buf_kern = path_buf,
				.len = len,
			},
			.caller = proc_cur,
			.backend = mount->backend,
			.flags = flags,
		});
	return -1; // dummy
fail:
	kfree(path_buf);
	SYSCALL_RETURN(-1);
}

long _sys_mount(hid_t hid, const char __user *path, long len) {
	VfsMount *mount = NULL;
	VfsBackend *backend = NULL;
	char *path_buf = NULL;

	if (PATH_MAX < len)
		SYSCALL_RETURN(-1);

	path_buf = kmalloc(len);
	if (pcpy_from(proc_cur, path_buf, path, len) < (size_t)len) {
		goto fail;
	}

	len = path_simplify(path_buf, path_buf, len);
	if (len == 0) goto fail;

	// remove trailing slash
	// mounting something under `/this` and `/this/` should be equalivent
	if (path_buf[len - 1] == '/') {
		if (len == 0) goto fail;
		len--;
	}

	Handle *handle = proc_handle_get(proc_cur, hid);
	if (!handle || handle->type != HANDLE_FS_FRONT)
		goto fail;
	backend = handle->backend;
	if (backend) {
		assert(backend->usehcnt);
		backend->usehcnt++;
	}

	// append to mount list
	// TODO move to kernel/vfs/mount.c
	mount = kmalloc(sizeof *mount);
	mount->prev = proc_cur->mount;
	mount->prefix = path_buf;
	mount->prefix_owned = true;
	mount->prefix_len = len;
	mount->backend = backend;
	mount->refs = 1;
	proc_cur->mount = mount;

	kmalloc_sanity(mount);
	kmalloc_sanity(mount->prefix);
	SYSCALL_RETURN(0);

fail:
	kfree(path_buf);
	kfree(mount);
	SYSCALL_RETURN(-1);
}

hid_t _sys_dup(hid_t from, hid_t to, int flags) {
	if (flags != 0) SYSCALL_RETURN(-ENOSYS);
	SYSCALL_RETURN(proc_handle_dup(proc_cur, from, to));
}

static long simple_vfsop(
		enum vfs_op vfsop, hid_t hid, void __user *buf,
		size_t len, long offset, int flags)
{
	assert(vfsop == VFSOP_READ
		|| vfsop == VFSOP_WRITE
		|| vfsop == VFSOP_GETSIZE);
	Handle *h = proc_handle_get(proc_cur, hid);
	if (!h) SYSCALL_RETURN(-EBADF);
	// TODO those checks really need some comprehensive tests
	if (vfsop == VFSOP_READ && !h->readable)
		SYSCALL_RETURN(-EACCES);
	if (vfsop == VFSOP_WRITE && !h->writeable)
		SYSCALL_RETURN(-EACCES);

	if (h->type == HANDLE_FILE) {
		VfsReq req = (VfsReq){
			.type = vfsop,
			.backend = h->backend,
			.id = h->file_id,
			.caller = proc_cur,
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
			/* already checked if this is the correct pipe end */
			pipe_joinqueue(h, proc_cur, buf, len);
		} else SYSCALL_RETURN(-ENOSYS);
	} else SYSCALL_RETURN(-ENOSYS);
	return 0;
}

long _sys_read(hid_t hid, void __user *buf, size_t len, long offset) {
	simple_vfsop(VFSOP_READ, hid, buf, len, offset, 0);
	return 0;
}

long _sys_write(hid_t hid, const void __user *buf, size_t len, long offset, int flags) {
	if (flags & ~(WRITE_TRUNCATE))
		SYSCALL_RETURN(-ENOSYS);
	simple_vfsop(VFSOP_WRITE, hid, (userptr_t)buf, len, offset, flags);
	return 0;
}

long _sys_getsize(hid_t hid) {
	simple_vfsop(VFSOP_GETSIZE, hid, NULL, 0, 0, 0);
	return 0;
}

long _sys_remove(hid_t hid) {
	Handle *h = proc_handle_get(proc_cur, hid);
	if (!h) SYSCALL_RETURN(-EBADF);
	if (h->type != HANDLE_FILE) {
		proc_handle_close(proc_cur, hid);
		SYSCALL_RETURN(-ENOSYS);
	}
	if (!h->writeable) {
		proc_handle_close(proc_cur, hid);
		SYSCALL_RETURN(-EACCES);
	}
	vfsreq_create((VfsReq) {
			.type = VFSOP_REMOVE,
			.id = h->file_id,
			.caller = proc_cur,
			.backend = h->backend,
		});
	proc_handle_close(proc_cur, hid);
	return -1; // dummy
}

long _sys_close(hid_t hid) {
	if (!proc_handle_get(proc_cur, hid)) {
		SYSCALL_RETURN(-EBADF);
	}
	proc_handle_close(proc_cur, hid);
	SYSCALL_RETURN(0);
}

hid_t _sys_fs_wait(char __user *buf, long max_len, struct ufs_request __user *res) {
	VfsBackend *backend = proc_cur->controlled;
	// TODO can be used to tell if you're init
	if (!backend) SYSCALL_RETURN(-1);
	if (backend->usehcnt == 0) {
		/* nothing on the other end. EPIPE seems fitting */
		SYSCALL_RETURN(-EPIPE);
	}

	proc_setstate(proc_cur, PS_WAITS4REQUEST);
	if (backend->user.handler)
		panic_unimplemented();
	backend->user.handler = proc_cur;
	proc_cur->awaited_req.buf     = buf;
	proc_cur->awaited_req.max_len = max_len;
	proc_cur->awaited_req.res     = res;

	vfs_backend_tryaccept(backend); // sets return value
	return -1; // dummy
}

long _sys_fs_respond(hid_t hid, const void __user *buf, long ret, int flags) {
	Handle *h = proc_handle_get(proc_cur, hid);
	if (!h || h->type != HANDLE_FS_REQ) SYSCALL_RETURN(-EBADF);
	VfsReq *req = h->req;
	if (req) {
		if (req->output.len > 0 && ret > 0) {
			// if this vfsop outputs data and ret is positive, it's the length of the buffer
			// TODO document
			// TODO move to vfsreq_finish
			ret = min(ret, capped_cast32(req->output.len));
			ret = pcpy_bi(
				req->caller, req->output.buf,
				proc_cur, buf,
				ret
			);
		}
		vfsreq_finish(req, (void __user *)buf, ret, flags, proc_cur);
	}
	h->req = NULL;
	proc_handle_close(proc_cur, hid);
	SYSCALL_RETURN(0);
}

void __user *_sys_memflag(void __user *addr, size_t len, int flags) {
	Pagedir *pages = proc_cur->pages;
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

long _sys_pipe(hid_t __user user_ends[2], int flags) {
	if (flags) SYSCALL_RETURN(-ENOSYS);

	hid_t ends[2];
	Handle *rend, *wend;
	ends[0] = proc_handle_init(proc_cur, HANDLE_PIPE, &rend);
	ends[1] = proc_handle_init(proc_cur, HANDLE_PIPE, &wend);
	if (ends[0] < 0 || ends[1] < 0) {
		proc_handle_close(proc_cur, ends[0]);
		proc_handle_close(proc_cur, ends[1]);
		SYSCALL_RETURN(-EMFILE);
	}
	wend->pipe.sister = rend;
	rend->pipe.sister = wend;
	wend->writeable = true;
	rend->readable = true;

	pcpy_to(proc_cur, user_ends, ends, sizeof ends);
	SYSCALL_RETURN(0);
}

void _sys_sleep(long ms) {
	// TODO no overflow check - can leak current uptime
	timer_schedule(proc_cur, uptime_ms() + ms);
}

void _sys_filicide(void) {
	proc_filicide(proc_cur, -1);
}

void _sys_intr(void) {
	for (Proc *p = proc_cur->child; p; p = proc_next(p, proc_cur)) {
		proc_intr(p);
	}
}

void _sys_intr_set(void __user *ip) {
	proc_cur->intr_fn = ip;
}

long _sys_execbuf(void __user *ubuf, size_t len) {
	if (len == 0) SYSCALL_RETURN(0);
	if (len > EXECBUF_MAX_LEN)
		SYSCALL_RETURN(-1);
	if (proc_cur->execbuf.buf)
		SYSCALL_RETURN(-1);
	// TODO consider supporting nesting execbufs

	char *kbuf = kmalloc(len);
	if (pcpy_from(proc_cur, kbuf, ubuf, len) < len) {
		kfree(kbuf);
		SYSCALL_RETURN(-1);
	}
	proc_cur->execbuf.buf = kbuf;
	proc_cur->execbuf.len = len;
	proc_cur->execbuf.pos = 0;
	SYSCALL_RETURN(0);
}

void _sys_debug_klog(const void __user *buf, size_t len) {
	if (false) {
		static char kbuf[256];
		if (len >= sizeof(kbuf)) len = sizeof(kbuf) - 1;
		pcpy_from(proc_cur, kbuf, buf, len);
		kbuf[len] = '\0';
		kprintf("[klog] %x\t%s\n", proc_cur->globalid, kbuf);
	}
}

long _syscall(long num, long a, long b, long c, long d, long e) {
	/* note: this isn't the only place where syscalls get called from!
	 *       see execbuf */
	switch (num) {
		break; case _SYS_EXIT:	_sys_exit(a);
		break; case _SYS_AWAIT:	_sys_await();
		break; case _SYS_FORK:	_sys_fork(a, (userptr_t)b);
		break; case _SYS_OPEN:	_sys_open((userptr_t)a, b, c);
		break; case _SYS_MOUNT:	_sys_mount(a, (userptr_t)b, c);
		break; case _SYS_DUP:	_sys_dup(a, b, c);
		break; case _SYS_READ:	_sys_read(a, (userptr_t)b, c, d);
		break; case _SYS_WRITE:	_sys_write(a, (userptr_t)b, c, d, e);
		break; case _SYS_GETSIZE:	_sys_getsize(a);
		break; case _SYS_REMOVE:	_sys_remove(a);
		break; case _SYS_CLOSE:	_sys_close(a);
		break; case _SYS_FS_WAIT:	_sys_fs_wait((userptr_t)a, b, (userptr_t)c);
		break; case _SYS_FS_RESPOND:	_sys_fs_respond(a, (userptr_t)b, c, d);
		break; case _SYS_MEMFLAG:	_sys_memflag((userptr_t)a, b, c);
		break; case _SYS_PIPE:	_sys_pipe((userptr_t)a, b);
		break; case _SYS_SLEEP:	_sys_sleep(a);
		break; case _SYS_FILICIDE:	_sys_filicide();
		break; case _SYS_INTR:	_sys_intr();
		break; case _SYS_INTR_SET:	_sys_intr_set((userptr_t)a);
		break; case _SYS_EXECBUF:	_sys_execbuf((userptr_t)a, b);
		break; case _SYS_DEBUG_KLOG:	_sys_debug_klog((userptr_t)a, b);
		break;
		default:
			regs_savereturn(&proc_cur->regs, -1);
			break;
	}
	/* return value is unused. execution continues in sysenter_stage2 */
	return -1;
}

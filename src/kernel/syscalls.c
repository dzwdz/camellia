#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/path.h>
#include <shared/flags.h>
#include <shared/mem.h>
#include <shared/syscalls.h>
#include <stdint.h>

_Noreturn void _syscall_exit(int ret) {
	process_kill(process_current, ret);
	process_switch_any();
}

int _syscall_await(void) {
	bool has_children = false;
	process_current->state = PS_WAITS4CHILDDEATH;

	// find any already dead children
	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling) {
		if (iter->state == PS_DEAD)
			return process_try2collect(iter);
		if (iter->state != PS_DEADER)
			has_children = true;
	}

	if (has_children)
		process_switch_any(); // wait until a child dies
	else {
		process_current->state = PS_RUNNING;
		return ~0; // TODO errno
	}
}

int _syscall_fork(void) {
	struct process *child = process_fork(process_current);
	regs_savereturn(&child->regs, 0);
	return 1;
}

handle_t _syscall_open(const char __user *path, int len) {
	struct vfs_mount *mount;
	char *path_buf = NULL;

	if (PATH_MAX < len)
		return -1;
	if (process_find_handle(process_current) < 0)
		return -1;

	path_buf = virt_cpy2kmalloc(process_current->pages, path, len);
	if (!path_buf) goto fail;

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

	return vfs_request_create((struct vfs_request) {
			.type = VFSOP_OPEN,
			.input = {
				.kern = true,
				.buf_kern = path_buf,
				.len = len,
			},
			.caller = process_current,
			.backend = mount->backend,
		});
fail:
	kfree(path_buf);
	return -1;
}

int _syscall_mount(handle_t handle, const char __user *path, int len) {
	struct vfs_mount *mount = NULL;
	struct vfs_backend *backend = NULL;
	char *path_buf = NULL;

	if (PATH_MAX < len) return -1;

	// copy the path to the kernel to simplify it
	path_buf = virt_cpy2kmalloc(process_current->pages, path, len);
	if (!path_buf) goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;

	// remove trailing slash
	// mounting something under `/this` and `/this/` should be equalivent
	if (path_buf[len - 1] == '/') {
		if (len == 0) goto fail;
		len--;
	}

	if (handle >= 0) { // mounting a real backend?
		if (handle >= HANDLE_MAX)
			goto fail;
		if (process_current->handles[handle].type != HANDLE_FS_FRONT)
			goto fail;
		backend = process_current->handles[handle].fs.backend;
	} // otherwise backend == NULL

	// append to mount list
	mount = kmalloc(sizeof *mount);
	mount->prev = process_current->mount;
	mount->prefix = path_buf;
	mount->prefix_len = len;
	mount->backend = backend;
	process_current->mount = mount;
	return 0;

fail:
	kfree(path_buf);
	kfree(mount);
	return -1;
}

int _syscall_read(handle_t handle_num, void __user *buf, int len, int offset) {
	struct handle *handle = &process_current->handles[handle_num];
	if (handle_num < 0 || handle_num >= HANDLE_MAX) return -1;
	if (handle->type != HANDLE_FILE) return -1;
	return vfs_request_create((struct vfs_request) {
			.type = VFSOP_READ,
			.output = {
				.buf = (userptr_t) buf,
				.len = len,
			},
			.id = handle->file.id,
			.offset = offset,
			.caller = process_current,
			.backend = handle->file.backend,
		});
}

int _syscall_write(handle_t handle_num, const void __user *buf, int len, int offset) {
	struct handle *handle = &process_current->handles[handle_num];
	if (handle_num < 0 || handle_num >= HANDLE_MAX) return -1;
	if (handle->type != HANDLE_FILE) return -1;
	return vfs_request_create((struct vfs_request) {
			.type = VFSOP_WRITE,
			.input = {
				.buf = (userptr_t) buf,
				.len = len,
			},
			.id = handle->file.id,
			.offset = offset,
			.caller = process_current,
			.backend = handle->file.backend,
		});
}

int _syscall_close(handle_t handle) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

handle_t _syscall_fs_fork2(void) {
	struct vfs_backend *backend;
	struct process *child;
	handle_t front;

	front = process_find_handle(process_current);
	if (front < 0) return -1;
	process_current->handles[front].type = HANDLE_FS_FRONT;

	/* so this can't return 0 in the parent or it will make it think that it's
	 * the child. i could make fork()s return -1 in the child but that's weird.
	 *
	 * also there's this whole thing with handling errors here properly and
	 * errno
	 * TODO figure this out */
	if (front == 0) {
		// dumb hack
		front = process_find_handle(process_current);
		if (front < 0) return -1;
		process_current->handles[front].type = HANDLE_FS_FRONT;
		process_current->handles[0].type = HANDLE_EMPTY;
	}

	backend = kmalloc(sizeof *backend); // TODO never freed
	backend->type = VFS_BACK_USER;
	backend->handler = NULL;
	backend->queue = NULL;

	child = process_fork(process_current);
	child->controlled = backend;
	regs_savereturn(&child->regs, 0);

	process_current->handles[front].fs.backend = backend;
	return front;
}

int _syscall_fs_wait(char __user *buf, int max_len, struct fs_wait_response __user *res) {
	struct vfs_backend *backend = process_current->controlled;
	if (!backend) return -1;

	process_current->state = PS_WAITS4REQUEST;
	backend->handler = process_current;
	/* checking the validity of those pointers here would make
	 * vfs_request_accept simpler. TODO? */
	process_current->awaited_req.buf     = buf;
	process_current->awaited_req.max_len = max_len;
	process_current->awaited_req.res     = res;

	if (backend->queue) {
		// handle queued requests
		struct process *queued = backend->queue;
		backend->queue = queued->waits4fs.queue_next;
		return vfs_request_accept(&queued->waits4fs.req);
	} else {
		process_switch_any();
	}
}

int _syscall_fs_respond(char __user *buf, int ret) {
	struct vfs_request *req = process_current->handled_req;
	if (!req) return -1;

	if (req->output.len > 0 && ret > 0) {
		// if this vfsop outputs data and ret is positive, it's the length of the buffer
		// TODO document
		if (ret > req->output.len)
			ret = req->output.len; // i'm not handling this in a special way - the fs server can prevent this on its own
		if (!virt_cpy(req->caller->pages, req->output.buf,
					process_current->pages, buf, ret)) {
			// how should this error even be handled? TODO
		}
	}

	process_current->handled_req = NULL;
	vfs_request_finish(req, ret);
	return 0;
}

int _syscall_memflag(void __user *addr, size_t len, int flags) {
	userptr_t goal = addr + len;
	struct pagedir *pages = process_current->pages;
	if (flags != MEMFLAG_PRESENT) panic_unimplemented(); // TODO

	addr = (userptr_t)((int __force)addr & ~PAGE_MASK); // align to page boundary
	for (; addr < goal; addr += PAGE_SIZE) {
		if (pagedir_virt2phys(pages, addr, false, false)) {
			// allocated page, currently a no-op
			// if you'll be changing this - remember to check if the pages are owned by the kernel!
		} else {
			// allocate the new pages
			pagedir_map(pages, addr, page_alloc(1), true, true);
		}
	}

	return -1;
}

int _syscall(int num, int a, int b, int c, int d) {
	switch (num) {
		case _SYSCALL_EXIT:
			_syscall_exit(a);
		case _SYSCALL_AWAIT:
			return _syscall_await();
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_OPEN:
			return _syscall_open((userptr_t)a, b);
		case _SYSCALL_MOUNT:
			return _syscall_mount(a, (userptr_t)b, c);
		case _SYSCALL_READ:
			return _syscall_read(a, (userptr_t)b, c, d);
		case _SYSCALL_WRITE:
			return _syscall_write(a, (userptr_t)b, c, d);
		case _SYSCALL_CLOSE:
			return _syscall_close(a);
		case _SYSCALL_FS_FORK2:
			return _syscall_fs_fork2();
		case _SYSCALL_FS_WAIT:
			return _syscall_fs_wait((userptr_t)a, b, (userptr_t)c);
		case _SYSCALL_FS_RESPOND:
			return _syscall_fs_respond((userptr_t)a, b);
		case _SYSCALL_MEMFLAG:
			return _syscall_memflag((userptr_t)a, b, c);
		default:
			tty_const("unknown syscall ");
			panic_unimplemented(); // TODO fail gracefully
	}
}

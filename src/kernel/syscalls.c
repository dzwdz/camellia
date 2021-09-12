#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/types.h>
#include <kernel/vfs/path.h>
#include <shared/syscalls.h>
#include <stdint.h>

_Noreturn static void await_finish(struct process *dead, struct process *listener) {
	int len;
	bool res;

	assert(dead->state == PS_DEAD);
	assert(listener->state == PS_WAITS4CHILDDEATH);
	dead->state = PS_DEADER;
	listener->state = PS_RUNNING;

	len = listener->death_msg.len < dead->death_msg.len
	    ? listener->death_msg.len : dead->death_msg.len;
	res = virt_cpy(
			listener->pages, listener->death_msg.buf,
			dead->pages, dead->death_msg.buf, len);
	regs_savereturn(&listener->regs, res ? len : -1);
	process_switch(listener);
}


_Noreturn void _syscall_exit(const char __user *msg, size_t len) {
	process_current->state = PS_DEAD;
	process_current->death_msg.buf = (userptr_t) msg; // discarding const
	process_current->death_msg.len = len;

	if (process_current->parent->state == PS_WAITS4CHILDDEATH)
		await_finish(process_current, process_current->parent);

	process_switch_any();
}

int _syscall_await(char __user *buf, int len) {
	process_current->state = PS_WAITS4CHILDDEATH;
	process_current->death_msg.buf = buf;
	process_current->death_msg.len = len;

	// find any already dead children
	for (struct process *iter = process_current->child;
			iter; iter = iter->sibling)
	{
		if (iter->state == PS_DEAD)
			await_finish(iter, process_current); // doesn't return
	}

	// no dead children yet
	// TODO check if the process even has children
	
	process_switch_any();
}

int _syscall_fork(void) {
	struct process *child = process_fork(process_current);
	regs_savereturn(&child->regs, 0);
	return 1;
}

handle_t _syscall_open(const char __user *path, int len) {
	struct vfs_mount *mount;
	char *path_buf = NULL;

	if (len > PATH_MAX)
		return -1;

	// fail if there are no handles left
	if (process_find_handle(process_current) < 0)
		return -1;

	// copy the path to the kernel
	// path_buf gets freed in vfs_request_finish
	path_buf = kmalloc(len);
	if (!virt_cpy_from(process_current->pages, path_buf, path, len))
		goto fail;

	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;

	mount = vfs_mount_resolve(process_current->mount, path_buf, len);
	if (!mount) goto fail;

	vfs_request_create((struct vfs_request) {
			.type = VFSOP_OPEN,
			.open = {
				.path     = &path_buf[mount->prefix_len],
				.path_len = len - mount->prefix_len,
			},

			.caller = process_current,
			.backend = mount->backend,
		});
	// doesn't return / fallthrough to fail

fail:
	kfree(path_buf);
	return -1;
}

int _syscall_mount(handle_t handle, const char __user *path, int len) {
	struct vfs_mount *mount = NULL;
	char *path_buf;

	if (len > PATH_MAX) return -1;
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	if (process_current->handles[handle].type != HANDLE_FS_FRONT)
		return -1;

	// copy the path to the kernel
	path_buf = kmalloc(len);
	if (!virt_cpy_from(process_current->pages, path_buf, path, len))
		goto fail;

	// simplify it
	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;
	// TODO remove trailing slash

	// append to mount list
	mount = kmalloc(sizeof *mount);
	mount->prev = process_current->mount;
	mount->prefix = path_buf;
	mount->prefix_len = len;
	mount->backend = process_current->handles[handle].fs.backend;
	process_current->mount = mount;
fail:
	kfree(path_buf);
	kfree(mount);
	return -1;
}

int _syscall_read(handle_t handle, char __user *buf, int len) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

int _syscall_write(handle_t handle_num, const char __user *buf, int len) {
	struct handle *handle = &process_current->handles[handle_num];
	if (handle_num < 0 || handle_num >= HANDLE_MAX) return -1;
	if (handle->type != HANDLE_FILE) return -1;
	vfs_request_create((struct vfs_request) {
			.type = VFSOP_WRITE,
			.rw = {
				.buf = (userptr_t) buf,
				.buf_len = len,
				.id = handle->file.id,
			},
			.caller = process_current,
			.backend = handle->file.backend,
		});
	return -1;
}

int _syscall_close(handle_t handle) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

handle_t _syscall_fs_create(handle_t __user *back_user) {
	handle_t front, back = 0;
	struct vfs_backend *backend;

	front = process_find_handle(process_current);
	if (front < 0) goto fail;
	// the type needs to be set here so process_find_handle skips this handle
	process_current->handles[front].type = HANDLE_FS_FRONT;

	back  = process_find_handle(process_current);
	if (back < 0) goto fail;
	process_current->handles[back].type = HANDLE_FS_BACK;

	// copy the back handle to back_user
	if (!virt_cpy_to(process_current->pages, back_user, &back, sizeof(back)))
		goto fail;

	backend = kmalloc(sizeof *backend); // TODO never freed
	backend->type = VFS_BACK_USER;
	backend->handler = NULL;
	backend->queue = NULL;

	process_current->handles[front].fs.backend = backend;
	process_current->handles[back ].fs.backend = backend;

	return front;
fail:
	if (front >= 0)
		process_current->handles[front].type = HANDLE_EMPTY;
	if (back >= 0)
		process_current->handles[back].type = HANDLE_EMPTY;
	return -1;
}

int _syscall_fs_wait(handle_t back, void __user *info) {
	struct handle *back_handle;

	if (back < 0 || back >= HANDLE_MAX) return -1;
	back_handle = &process_current->handles[back];
	if (back_handle->type != HANDLE_FS_BACK)
		return -1;

	process_current->state = PS_WAITS4REQUEST;
	back_handle->fs.backend->handler = process_current;

	if (back_handle->fs.backend->queue) {
		// handle queued requests
		struct process *queued = back_handle->fs.backend->queue;
		back_handle->fs.backend->queue = NULL; // TODO get the next queued proc
		vfs_request_pass2handler(&queued->pending_req);
	} else {
		process_switch_any();
	}
}

int syscall_handler(int num, int a, int b, int c) {
	switch (num) {
		case _SYSCALL_EXIT:
			_syscall_exit((userptr_t)a, b);
		case _SYSCALL_AWAIT:
			return _syscall_await((userptr_t)a, b);
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_OPEN:
			return _syscall_open((userptr_t)a, b);
		case _SYSCALL_MOUNT:
			return _syscall_mount(a, (userptr_t)b, c);
		case _SYSCALL_READ:
			return _syscall_read(a, (userptr_t)b, c);
		case _SYSCALL_WRITE:
			return _syscall_write(a, (userptr_t)b, c);
		case _SYSCALL_CLOSE:
			return _syscall_close(a);
		case _SYSCALL_FS_CREATE:
			return _syscall_fs_create((userptr_t)a);
		case _SYSCALL_FS_WAIT:
			return _syscall_fs_wait(a, (userptr_t)b);
		default:
			tty_const("unknown syscall ");
			panic();
	}
}

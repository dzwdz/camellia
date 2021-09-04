#include <kernel/arch/generic.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/types.h>
#include <kernel/vfs/path.h>
#include <shared/syscalls.h>
#include <stdint.h>

_Noreturn static void await_finish(struct process *dead, struct process *listener) {
	size_t len;
	bool res;

	assert(dead->state == PS_DEAD);
	assert(listener->state == PS_WAITS4CHILDDEATH);
	dead->state = PS_DEADER;
	listener->state = PS_RUNNING;

	len = listener->saved_len < dead->saved_len
	    ? listener->saved_len : dead->saved_len;
	res = virt_user_cpy(
			listener->pages, listener->saved_addr,
			dead->pages, dead->saved_addr, len);
	regs_savereturn(&listener->regs, res ? len : -1);
	process_switch(listener);
}


_Noreturn void _syscall_exit(const user_ptr msg, size_t len) {
	process_current->state = PS_DEAD;
	process_current->saved_addr = msg;
	process_current->saved_len  = len;

	if (process_current->parent->state == PS_WAITS4CHILDDEATH)
		await_finish(process_current, process_current->parent);

	process_current = process_find(PS_RUNNING);
	if (process_current)
		process_switch(process_current);

	process_switch_any();
}

int _syscall_await(user_ptr buf, int len) {
	process_current->state = PS_WAITS4CHILDDEATH;
	process_current->saved_addr = buf;
	process_current->saved_len  = len;

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

handle_t _syscall_fs_open(const user_ptr path, int len) {
	struct virt_iter iter;
	struct vfs_mount *mount;
	static char buffer[PATH_MAX]; // holds the path
	int handle, res;

	if (len > PATH_MAX) return -1;

	// find the first free handle
	handle = process_find_handle(process_current);

	// copy the path to the kernel
	virt_iter_new(&iter, path, len, process_current->pages, true, false);
	while (virt_iter_next(&iter))
		memcpy(buffer + iter.prior, iter.frag, iter.frag_len);
	if (iter.error) return -1;

	len = path_simplify(buffer, buffer, len);
	if (len < 0) return -1;

	mount = vfs_mount_resolve(process_current->mount, buffer, len);
	if (!mount) return -1;

	res = -1; // TODO pass to filesystem
	if (res < 0)
		return res;
	else
		return handle;
}

int _syscall_fd_mount(handle_t handle, const user_ptr path, int len) {
	struct virt_iter iter;
	struct vfs_mount *mount;
	char *path_buf;
	int res;

	if (len > PATH_MAX) return -1;
	if (handle < 0 || handle >= HANDLE_MAX) return -1;

	// copy the path to the kernel
	path_buf = kmalloc(len);
	virt_iter_new(&iter, path, len, process_current->pages, true, false);
	while (virt_iter_next(&iter))
		memcpy(path_buf + iter.prior, iter.frag, iter.frag_len);
	if (iter.error) goto fail;

	// simplify it
	len = path_simplify(path_buf, path_buf, len);
	if (len < 0) goto fail;
	// TODO remove trailing slash

	// append to mount list
	mount = kmalloc(sizeof(struct vfs_mount));
	mount->prev = process_current->mount;
	mount->prefix = path_buf;
	mount->prefix_len = len;

	res = -1; // TODO pass to filesystem
	if (res < 0) goto fail;
	process_current->mount = mount;
	return 0;
fail:
	kfree(path_buf);
	kfree(mount);
	return -1;
}

int _syscall_fd_read(handle_t handle, user_ptr buf, int len) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

int _syscall_fd_write(handle_t handle, user_ptr buf, int len) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

int _syscall_fd_close(handle_t handle) {
	if (handle < 0 || handle >= HANDLE_MAX) return -1;
	return -1;
}

int syscall_handler(int num, int a, int b, int c) {
	switch (num) {
		case _SYSCALL_EXIT:
			_syscall_exit(a, b);
		case _SYSCALL_AWAIT:
			return _syscall_await(a, b);
		case _SYSCALL_FORK:
			return _syscall_fork();
		case _SYSCALL_FS_OPEN:
			return _syscall_fs_open(a, b);
		case _SYSCALL_FD_MOUNT:
			return _syscall_fd_mount(a, b, c);
		case _SYSCALL_FD_READ:
			return _syscall_fd_read(a, b, c);
		case _SYSCALL_FD_WRITE:
			return _syscall_fd_write(a, b, c);
		case _SYSCALL_FD_CLOSE:
			return _syscall_fd_close(a);
		default:
			tty_const("unknown syscall ");
			panic();
	}
}

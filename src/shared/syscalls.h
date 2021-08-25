// requires the user_ptr type from kernel/types.h or init/types.h

#pragma once
#include <stddef.h>

typedef int fd_t;
typedef int fs_handle_t;

enum {
	// idc about stable syscall numbers just yet
	_SYSCALL_EXIT,
	_SYSCALL_AWAIT,
	_SYSCALL_FORK,

	_SYSCALL_FS_OPEN,
	_SYSCALL_FD_MOUNT,

	_SYSCALL_FD_READ,
	_SYSCALL_FD_WRITE,
	_SYSCALL_FD_CLOSE,
};

/** Kills the current process.
 * TODO: what happens to the children?
 */
_Noreturn void _syscall_exit(const user_ptr msg, size_t len);

/** Waits for a child to exit, putting its exit message into *buf.
 * @return the length of the message
 */
int _syscall_await(user_ptr buf, int len);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 * @return 0 in the child, a meaningless positive value in the parent.
 */
int _syscall_fork(void);

fd_t _syscall_fs_open(const user_ptr path, int len);

int _syscall_fd_mount(fd_t fd, const user_ptr path, int len);
int _syscall_fd_read(fd_t fd, user_ptr buf, int len);
int _syscall_fd_write(fd_t fd, user_ptr buf, int len);
int _syscall_fd_close(fd_t fd);

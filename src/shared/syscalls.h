#pragma once
#include <stddef.h>

#ifndef TYPES_INCLUDED
#  error "please include <kernel/types.h> or <init/types.h> before this file"
#endif

typedef int handle_t;

enum {
	// idc about stable syscall numbers just yet
	_SYSCALL_EXIT,
	_SYSCALL_AWAIT,
	_SYSCALL_FORK,

	_SYSCALL_OPEN,
	_SYSCALL_MOUNT,

	_SYSCALL_READ,
	_SYSCALL_WRITE,
	_SYSCALL_CLOSE,

	_SYSCALL_FS_CREATE,
	_SYSCALL_FS_WAIT,
	_SYSCALL_FS_RESPOND,

	_SYSCALL_MEMFLAG,
};

/** Kills the current process.
 * TODO: what happens to the children?
 */
_Noreturn void _syscall_exit(const char __user *msg, size_t len);

/** Waits for a child to exit, putting its exit message into *buf.
 * @return the length of the message
 */
int _syscall_await(char __user *buf, int len);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 * @return 0 in the child, a meaningless positive value in the parent.
 */
int _syscall_fork(void);

handle_t _syscall_open(const char __user *path, int len);

int _syscall_mount(handle_t, const char __user *path, int len);
int _syscall_read(handle_t, char __user *buf, int len);
int _syscall_write(handle_t, const char __user *buf, int len);
int _syscall_close(handle_t);

/** Creates a pair of front/back filesystem handles.
 * @param back a pointer to a handle_t which will store the back pointer
 */
handle_t _syscall_fs_create(handle_t __user *back);
int _syscall_fs_wait(handle_t back, char __user *buf, int __user *len);
int _syscall_fs_respond(char __user *buf, int ret);

int _syscall_memflag(void __user *addr, size_t len, int flags);

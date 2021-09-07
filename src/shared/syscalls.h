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

handle_t _syscall_open(const user_ptr path, int len);

int _syscall_mount(handle_t, const user_ptr path, int len);
int _syscall_read(handle_t, user_ptr buf, int len);
int _syscall_write(handle_t, user_ptr buf, int len);
int _syscall_close(handle_t);

/** Creates a pair of front/back filesystem handles.
 * @param back a pointer to a handle_t which will store the back pointer
 */
handle_t _syscall_fs_create(user_ptr back);

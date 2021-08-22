// note: this file gets included in both kernel and userland
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
	_SYSCALL_MOUNT,

	_SYSCALL_DEBUGLOG
};

/** Kills the current process.
 * TODO: what happens to the children?
 */
_Noreturn void _syscall_exit(const char *msg, size_t len);

/** Waits for a child to exit, putting its exit message into *buf.
 * @return the length of the message
 */
int _syscall_await(char *buf, int len);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 * @return 0 in the child, a meaningless positive value in the parent.
 */
int _syscall_fork();

fd_t _syscall_fs_open(const char *path, int len);
int _syscall_mount(const char *path, int len, fd_t fd);

/** Prints a message to the debug console.
 * @return the amount of bytes written (can be less than len)
 */
int _syscall_debuglog(const char *msg, size_t len);

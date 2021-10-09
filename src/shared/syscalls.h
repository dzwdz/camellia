#pragma once
#include <shared/types.h>
#include <stddef.h>

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

int _syscall(int, int, int, int, int);

/** Kills the current process.
 * TODO: what happens to the children?
 */
_Noreturn void _syscall_exit(int ret);

/** Waits for a child to exit.
 * @return the value the child passed to exit()
 */
int _syscall_await(void);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 * @return 0 in the child, a meaningless positive value in the parent.
 */
int _syscall_fork(void);

handle_t _syscall_open(const char __user *path, int len);

int _syscall_mount(handle_t, const char __user *path, int len);
int _syscall_read(handle_t, char __user *buf, int len, int offset);
int _syscall_write(handle_t, const char __user *buf, int len, int offset);
int _syscall_close(handle_t);

/** Creates a pair of front/back filesystem handles.
 * @param back a pointer to a handle_t which will store the back pointer
 */
handle_t _syscall_fs_create(handle_t __user *back);
struct fs_wait_response {
	int len; // how much was put in *buf
	int id;  // file id (returned by the open handler, passed to other calls)
	int offset;
};
int _syscall_fs_wait(handle_t back, char __user *buf, int max_len,
		struct fs_wait_response __user *res);
int _syscall_fs_respond(char __user *buf, int ret);

int _syscall_memflag(void __user *addr, size_t len, int flags);

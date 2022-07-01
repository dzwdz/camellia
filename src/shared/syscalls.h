#pragma once
#include <shared/types.h>
#include <stddef.h>

#define FORK_NOREAP 1
#define FORK_NEWFS 2
#define OPEN_CREATE 1

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

	_SYSCALL_FS_FORK2,
	_SYSCALL_FS_WAIT,
	_SYSCALL_FS_RESPOND,

	_SYSCALL_MEMFLAG,

	_SYSCALL_DEBUG_KLOG,
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
 *
 * @param flags FORK_NOREAP, FORK_NEWFS
 * @param fs_front requires FORK_NEWFS. the front handle to the new fs is put there
 *
 * @return 0 in the child, a meaningless positive value in the parent.
 */
int _syscall_fork(int flags, handle_t __user *fs_front);

handle_t _syscall_open(const char __user *path, int len, int flags);

int _syscall_mount(handle_t h, const char __user *path, int len);
int _syscall_read(handle_t h, void __user *buf, size_t len, int offset);
int _syscall_write(handle_t h, const void __user *buf, size_t len, int offset);
int _syscall_close(handle_t h);

struct fs_wait_response {
	enum vfs_operation op;
	size_t len; // how much was put in *buf
	size_t capacity; // how much output can be accepted by the caller
	int id;  // file id (returned by the open handler, passed to other calls)
	int offset;
	int flags;
};
/** Blocks until an fs request is made.
 * @return 0 if everything was successful */
int _syscall_fs_wait(char __user *buf, int max_len, struct fs_wait_response __user *res);
int _syscall_fs_respond(char __user *buf, int ret);

/** Modifies the virtual address space.
 *
 * If the MEMFLAG_PRESENT flag is present - mark the memory region as allocated.
 * Otherwise, free it.
 *
 * MEMFLAG_FINDFREE tries to find the first free region of length `len`.
 *
 * @return address of the first affected page (usually == addr)
 */
void __user *_syscall_memflag(void __user *addr, size_t len, int flags);

void _syscall_debug_klog(const void __user *buf, size_t len);

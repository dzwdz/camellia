#pragma once

#define _SYSCALL_EXIT 0
#define _SYSCALL_AWAIT 1
#define _SYSCALL_FORK 2

#define _SYSCALL_OPEN 3
#define _SYSCALL_MOUNT 4
#define _SYSCALL_DUP 5

#define _SYSCALL_READ 6
#define _SYSCALL_WRITE 7
#define _SYSCALL_GETSIZE 8
#define _SYSCALL_REMOVE 9
#define _SYSCALL_CLOSE 10

#define _SYSCALL_FS_WAIT 11
#define _SYSCALL_FS_RESPOND 12

#define _SYSCALL_MEMFLAG 13
#define _SYSCALL_PIPE 14

#define _SYSCALL_SLEEP 15

#define _SYSCALL_EXECBUF 100

#define _SYSCALL_DEBUG_KLOG 101

#ifndef ASM_FILE
#include <camellia/types.h>
#include <stddef.h>

long _syscall(long, long, long, long, long, long);

/** Kills the current process.
 */
_Noreturn void _syscall_exit(long ret);

/** Waits for a child to exit.
 * @return the value the child passed to exit()
 */
long _syscall_await(void);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 *
 * @param flags FORK_NOREAP, FORK_NEWFS
 * @param fs_front requires FORK_NEWFS. the front handle to the new fs is put there
 *
 * @return 0 in the child, the CID in the parent.
 */
long _syscall_fork(int flags, handle_t __user *fs_front);

handle_t _syscall_open(const char __user *path, long len, int flags);
long _syscall_mount(handle_t h, const char __user *path, long len);
handle_t _syscall_dup(handle_t from, handle_t to, int flags);

long _syscall_read(handle_t h, void __user *buf, size_t len, long offset);
long _syscall_write(handle_t h, const void __user *buf, size_t len, long offset, int flags);
long _syscall_getsize(handle_t h);
long _syscall_remove(handle_t h);
long _syscall_close(handle_t h);

handle_t _syscall_fs_wait(char __user *buf, long max_len, struct fs_wait_response __user *res);
long _syscall_fs_respond(handle_t hid, const void __user *buf, long ret, int flags);

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
long _syscall_pipe(handle_t __user user_ends[2], int flags);

void _syscall_sleep(long ms);

/* see shared/execbuf.h */
long _syscall_execbuf(void __user *buf, size_t len);

void _syscall_debug_klog(const void __user *buf, size_t len);

#endif

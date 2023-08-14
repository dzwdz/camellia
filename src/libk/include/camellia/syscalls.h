#pragma once

#define _SYS_EXIT 0
#define _SYS_AWAIT 1
#define _SYS_FORK 2
#define _SYS_OPEN 3
#define _SYS_MOUNT 4
#define _SYS_DUP 5
#define _SYS_READ 6
#define _SYS_WRITE 7
#define _SYS_GETSIZE 8
#define _SYS_REMOVE 9
#define _SYS_CLOSE 10
#define _SYS_FS_WAIT 11
#define _SYS_FS_RESPOND 12
#define _SYS_MEMFLAG 13
#define _SYS_PIPE 14
#define _SYS_SLEEP 15
#define _SYS_FILICIDE 16
#define _SYS_INTR 17
#define _SYS_INTR_SET 18
#define _SYS_GETPID 19
#define _SYS_GETPPID 20
#define _SYS_WAIT2 21

#define _SYS_EXECBUF 100
#define _SYS_DEBUG_KLOG 101

#ifndef ASM_FILE
#include <camellia/types.h>
#include <stddef.h>

long _syscall(long, long, long, long, long, long);

/** Kills the current process.
 */
_Noreturn void _sys_exit(long ret);

/** Waits for a child to exit.
 * @return the value the child passed to exit()
 */
long _sys_await(void);

/** Creates a copy of the current process, and executes it.
 * All user memory pages get copied too.
 *
 * @param flags FORK_NOREAP, FORK_NEWFS
 * @param fs_front requires FORK_NEWFS. the front handle to the new fs is put there
 *
 * @return 0 in the child, the CID in the parent.
 */
long _sys_fork(int flags, hid_t __user *fs_front);

hid_t _sys_open(const char __user *path, long len, int flags);
long _sys_mount(hid_t h, const char __user *path, long len);
hid_t _sys_dup(hid_t from, hid_t to, int flags);

long _sys_read(hid_t h, void __user *buf, size_t len, long offset);
long _sys_write(hid_t h, const void __user *buf, size_t len, long offset, int flags);
long _sys_getsize(hid_t h);
long _sys_remove(hid_t h);
long _sys_close(hid_t h);

hid_t _sys_fs_wait(char __user *buf, long max_len, struct ufs_request __user *res);
long _sys_fs_respond(hid_t hid, const void __user *buf, long ret, int flags);

/** Modifies the virtual address space.
 *
 * If the MEMFLAG_PRESENT flag is present - mark the memory region as allocated.
 * Otherwise, free it.
 *
 * MEMFLAG_FINDFREE tries to find the first free region of length `len`.
 *
 * @return address of the first affected page (usually == addr)
 */
void __user *_sys_memflag(void __user *addr, size_t len, int flags);
long _sys_pipe(hid_t __user user_ends[2], int flags);

void _sys_sleep(long ms);

void _sys_filicide(void);
void _sys_intr(void);
void _sys_intr_set(void __user *ip);

uint32_t _sys_getpid(void);
uint32_t _sys_getppid(void);

// TODO deprecate await
int _sys_wait2(int pid, int flags, struct sys_wait2 __user *out);

/* see shared/execbuf.h */
long _sys_execbuf(void __user *buf, size_t len);

void _sys_debug_klog(const void __user *buf, size_t len);

#endif

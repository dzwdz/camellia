// this file could probably just get generated by a script
#include <kernel/syscalls.h>

int _syscall(int, int, int, int);

_Noreturn void _syscall_exit(const char *msg, size_t len) {
	_syscall(_SYSCALL_EXIT, (int)msg, len, 0);
	__builtin_unreachable();
}

int _syscall_fork() {
	return _syscall(_SYSCALL_FORK, 0, 0, 0);
}

int _syscall_await(char *buf, int len) {
	return _syscall(_SYSCALL_AWAIT, (int)buf, (int)len, 0);
}

fd_t _syscall_fs_open(const char *path, int len) {
	return _syscall(_SYSCALL_FS_OPEN, (int)path, len, 0);
}

int _syscall_mount(const char *path, int len, fd_t fd) {
	return _syscall(_SYSCALL_MOUNT, (int)path, len, fd);
}

int _syscall_debuglog(const char *msg, size_t len) {
	return _syscall(_SYSCALL_DEBUGLOG, (int)msg, len, 0);
}

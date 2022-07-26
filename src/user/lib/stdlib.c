#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <user/lib/elfload.h>

int errno = 0;

int fork(void) {
	return _syscall_fork(0, NULL);
}

int close(handle_t h) {
	return _syscall_close(h);
}

_Noreturn void exit(int c) {
	_syscall_exit(c);
}

int execv(const char *path, char *const argv[]) {
	(void)argv;

	FILE *file = fopen(path, "r");
	if (!file)
		return -1;

	elf_execf(file);
	fclose(file);
	errno = EINVAL;
	return -1;
}

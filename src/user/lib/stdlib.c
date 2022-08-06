#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
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
	FILE *file = fopen(path, "r");
	char hdr[4] = {0};
	if (!file)
		return -1;

	fread(hdr, 1, 4, file);
	fseek(file, 0, SEEK_SET);

	if (!memcmp("\x7f""ELF", hdr, 4)) {
		elf_execf(file, (void*)argv, NULL);
		fclose(file);
	} else if (!memcmp("#!", hdr, 2)) {
		char buf[256];
		fseek(file, 2, SEEK_SET);
		if (fgets(buf, sizeof buf, file)) {
			const char *argv [] = {buf, path, NULL};
			char *endl = strchr(buf, '\n');
			if (endl) *endl = '\0';
			execv(buf, (void*)argv);
		}
	}

	errno = EINVAL;
	return -1;
}

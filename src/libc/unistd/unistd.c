#include <bits/panic.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elfload.h>

int unlink(const char *path) {
	hid_t h = camellia_open(path, OPEN_WRITE);
	if (h < 0) return errno = -h, -1;
	long ret = _sys_remove(h);
	if (ret < 0) return errno = -ret, -1;
	return 0;
}

int rmdir(const char *path) {
	(void)path;
	__libc_panic("unimplemented");
}

int execv(const char *path, char *const argv[]) {
	return execve(path, argv, NULL);
}

int execvp(const char *path, char *const argv[]) {
	// TODO execvp
	return execve(path, argv, NULL);
}

int execvpe(const char *path, char *const argv[], char *const envp[]) {
	if (path[0] != '/') {
		char *exp = malloc(strlen(path) + 6);
		int ret;
		strcpy(exp, "/bin/");
		strcat(exp, path);
		ret = execve(exp, argv, envp);
		free(exp);
		return ret;
	}
	return execve(path, argv, envp);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
	FILE *file = fopen(path, "e");
	char hdr[4] = {0};
	if (!file)
		return -1;

	fread(hdr, 1, 4, file);
	fseek(file, 0, SEEK_SET);

	if (!memcmp("\x7f""ELF", hdr, 4)) {
		elf_execf(file, (void*)argv, (void*)envp);
		fclose(file);
	} else if (!memcmp("#!", hdr, 2)) {
		char buf[256];
		fseek(file, 2, SEEK_SET);
		if (fgets(buf, sizeof buf, file)) {
			const char *argv [] = {buf, path, NULL};
			char *endl = strchr(buf, '\n');
			if (endl) *endl = '\0';
			execve(buf, (void*)argv, envp);
		}
	}

	errno = EINVAL;
	return -1;
}

pid_t getpgrp(void) {
	__libc_panic("unimplemented");
}

int getgroups(int size, gid_t list[]) {
	(void)size; (void)list;
	__libc_panic("unimplemented");
}

int dup(int oldfd) {
	(void)oldfd;
	__libc_panic("unimplemented");
}

#include <_proc.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <string.h>
#include <elfload.h>
#include <camellia/fs/misc.h>

#include "tar.h"

extern char _bss_start;
extern char _bss_end;
extern char _initrd;

__attribute__((section(".text")))
int main(void) {
	_sys_memflag(_libc_psdata, 1, MEMFLAG_PRESENT);
	setprogname("bootstrap");
	setproctitle(NULL);

	/* move everything provided by the kernel to /kdev */
	MOUNT_AT("/kdev/") { fs_passthru(NULL); }
	MOUNT_AT("/") {
		const char *l[] = {"/kdev/", NULL};
		fs_whitelist(l);
	}

	_sys_mount(HANDLE_PROCFS, "/proc/", strlen("/proc/"));
	MOUNT_AT("/") { fs_dir_inject("/proc/"); }

	MOUNT_AT("/init/") { tar_driver(&_initrd); }

	const char *initpath = "bin/amd64/init";
	char *initargv[] = {"init", NULL};
	void *init = tar_find(initpath, strlen(initpath), &_initrd, ~0) + 512;
	if (init) {
		_klogf("execing init");
		elf_exec(init, initargv, NULL);
		_klogf("elf_exec failed");
	} else {
		_klogf("couldn't find init.elf");
	}
	_sys_exit(1);
}

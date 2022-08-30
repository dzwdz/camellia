#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <stdio.h>
#include <user/lib/elfload.h>
#include <user/lib/fs/misc.h>

#include "tar.h"

extern char _bss_start;
extern char _bss_end;
extern char _initrd;

__attribute__((section(".text")))
_Noreturn void main(void) {
	/* move everything provided by the kernel to /kdev */
	MOUNT_AT("/kdev/") { fs_passthru(NULL); }
	MOUNT_AT("/") {
		const char *l[] = {"/kdev/", NULL};
		fs_whitelist(l);
	}

	MOUNT_AT("/init/") { tar_driver(&_initrd); }

	const char *initpath = "bin/amd64/init";
	void *init = tar_find(initpath, strlen(initpath), &_initrd, ~0) + 512;
	if (init) {
		_klogf("execing init");
		elf_exec(init, NULL, NULL);
		_klogf("elf_exec failed");
	} else {
		_klogf("couldn't find init.elf");
	}
	_syscall_exit(1);
}

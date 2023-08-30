#include <_proc.h>
#include <camellia.h>
#include <camellia/flags.h>
#include <camellia/fs/misc.h>
#include <camellia/syscalls.h>
#include <elfload.h>
#include <stdio.h>
#include <string.h>

#include "tar.h"

extern char _bss_start;
extern char _bss_end;
extern char _initrd;

__attribute__((section(".text")))
int main(void) {
	_sys_memflag(_psdata_loc, 1, MEMFLAG_PRESENT);
	setprogname("bootstrap");

	camellia_procfs("/proc/");
	MOUNT_AT("/") {
		fs_dirinject2((const char*[]) {
			"/proc/",
			"/init/",
			NULL
		});
	}
	MOUNT_AT("/init/") {
		tar_driver(&_initrd);
	}

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

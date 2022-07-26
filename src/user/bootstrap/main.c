#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <user/lib/elfload.h>
#include <user/lib/fs/misc.h>

#include "tar.h"

extern char _bss_start;
extern char _bss_end;
extern char _initrd;

__attribute__((section(".text.startup")))
void _start(void) {
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	/* move everything provided by the kernel to /kdev */
	MOUNT("/kdev/", fs_passthru(NULL));
	if (!fork2_n_mount("/")) {
		const char *l[] = {"/kdev/", NULL};
		fs_whitelist(l);
	}
	if (!fork2_n_mount("/")) fs_dir_inject("/kdev/"); // TODO should be part of fs_whitelist

	MOUNT("/init/", tar_driver(&_initrd));

	void *init = tar_find("bin/init", 8, &_initrd, ~0) + 512;
	if (init) {
		_klogf("execing init.elf");
		elf_exec(init);
		_klogf("elf_exec failed");
	} else {
		_klogf("couldn't find init.elf");
	}
	_syscall_exit(1);
}

int main(void) {return 0;} // dummy, needed to link

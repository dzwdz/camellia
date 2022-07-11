#include <init/driver/driver.h>
#include <init/fs/misc.h>
#include <init/shell.h>
#include <init/stdlib.h>
#include <init/tar.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdint.h>

extern char _bss_start; // provided by the linker
extern char _bss_end;
extern char _initrd;

void read_file(const char *path, size_t len);

__attribute__((section(".text.startup")))
int main(void) {
	// allocate bss
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	stdout = file_open("/com1", 0);
	printf("preinit\n");

	/* move everything provided by the kernel to /kdev */
	MOUNT("/kdev/", fs_passthru(NULL));
	if (!fork2_n_mount("/")) {
		const char *l[] = {"/kdev/", NULL};
		fs_whitelist(l);
	}
	if (!fork2_n_mount("/")) fs_dir_inject("/kdev/"); // TODO should be part of fs_whitelist

	MOUNT("/init/", tar_driver(&_initrd));
	MOUNT("/tmp/", tmpfs_drv());
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());

	MOUNT("/bind/", fs_passthru(NULL));

	if (_syscall_fork(0, NULL)) {
		/* (used to) expose a bug in the kernel
		 * the program will flow like this:
		 * 1. we launch the forked init
		 * 2. the forked init launches both shells
		 * 3. one of the shells quit
		 * 4. the forked init picks it up and quits
		 *
		 * then, in process_kill, the other shell will be deathbedded
		 *
		 * before i implement(ed) reparenting, it was a lingering running child
		 * of a dead process, which is invalid state
		 */
		_syscall_await();
		_syscall_exit(1);
	}

	if (!_syscall_fork(0, NULL)) {
		libc_file *new = file_open("/kdev/com1", 0);
		if (!new) {
			printf("couldn't open /kdev/com1\n");
			_syscall_exit(1);
		}
		stdout = stdin = new; // TODO file_clone, this is bad

		shell_loop();
		_syscall_exit(1);
	}


	if (!_syscall_fork(0, NULL)) {
		libc_file *new;
		new = file_open("/vga_tty", 0);
		if (!new) {
			printf("couldn't open /vga_tty\n");
			_syscall_exit(1);
		}
		file_close(stdout);
		stdout = new;

		stdin = file_open("/keyboard", 0);
		if (!stdin) {
			printf("couldn't open /keyboard\n");
			_syscall_exit(1);
		}

		shell_loop();
		_syscall_exit(1);
	}

	_syscall_await();
	printf("init: quitting\n");
	_syscall_exit(0);
}

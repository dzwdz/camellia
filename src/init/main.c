#include <init/driver/driver.h>
#include <init/fs/misc.h>
#include <init/shell.h>
#include <init/stdlib.h>
#include <init/tar.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdint.h>

// TODO move to shared header file
#define argify(str) str, sizeof(str) - 1

extern char _bss_start; // provided by the linker
extern char _bss_end;
extern char _initrd;

void read_file(const char *path, size_t len);

__attribute__((section(".text.startup")))
int main(void) {
	// allocate bss
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	MOUNT("/init/", tar_driver(&_initrd));
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());

	MOUNT("/bind/", fs_passthru(NULL));

	if (_syscall_fork(0)) {
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

	if (!_syscall_fork(0)) {
		if (file_open(&__stdout, "/com1") < 0 || file_open(&__stdin, "/com1") < 0)
			_syscall_exit(1);

		shell_loop();
		_syscall_exit(1);
	}


	if (!_syscall_fork(0)) {
		if (file_open(&__stdout, "/vga_tty") < 0)
			_syscall_exit(1);

		if (file_open(&__stdin, "/keyboard") < 0) {
			printf("couldn't open /keyboard\n");
			_syscall_exit(1);
		}

		shell_loop();
		_syscall_exit(1);
	}


	// try to find any working output
	if (file_open(&__stdout, "/com1") < 0)
		file_open(&__stdout, "/vga_tty");

	_syscall_await();
	printf("init: quitting\n");
	_syscall_exit(0);
}

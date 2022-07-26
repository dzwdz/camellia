#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdint.h>
#include <user/app/init/driver/driver.h>
#include <user/app/init/shell.h>
#include <user/app/init/tests/main.h>
#include <user/lib/elfload.h>
#include <user/lib/fs/misc.h>
#include <user/lib/stdlib.h>

__attribute__((visibility("hidden")))
extern char _image_base[];

void read_file(const char *path, size_t len);

__attribute__((section(".text.startup")))
int main(void) {
	elf_selfreloc();

	file_reopen(stdout, "/kdev/com1", 0);
	printf("in init (stage 2), loaded at 0x%x\n", &_image_base);

	MOUNT("/tmp/", tmpfs_drv());
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());

	MOUNT("/bind/", fs_passthru(NULL));

	if (fork()) {
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

	if (!fork()) {
		if (!file_reopen(stdout, "/kdev/com1", 0)) {
			printf("couldn't open /kdev/com1\n"); // TODO borked
			_syscall_exit(1);
		}
		if (!file_reopen(stdin, "/kdev/com1", 0)) {
			printf("couldn't open /kdev/com1\n");
			_syscall_exit(1);
		}
		termcook();

		shell_loop();
		_syscall_exit(1);
	}

	if (!fork()) {
		if (!file_reopen(stdout, "/vga_tty", 0)) {
			printf("couldn't open /vga_tty\n"); // TODO borked
			_syscall_exit(1);
		}
		if (!file_reopen(stdin, "/keyboard", 0)) {
			printf("couldn't open /keyboard\n");
			_syscall_exit(1);
		}
		termcook();

		shell_loop();
		_syscall_exit(1);
	}

	_syscall_await();
	printf("init: quitting\n");
	_syscall_exit(0);
}

#include "driver/driver.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <user/lib/fs/misc.h>

int main(void) {
	freopen("/kdev/com1", "a+", stdout);
	printf("in init (stage 2), main at 0x%x\n", &main);

	MOUNT("/tmp/", tmpfs_drv());
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());
	MOUNT("/bin/", fs_passthru("/init/bin"));

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
		exit(1);
	}

	if (!fork()) {
		if (!freopen("/kdev/com1", "a+", stdout)) {
			printf("couldn't open /kdev/com1\n"); // TODO borked
			exit(1);
		}
		if (!freopen("/kdev/com1", "r", stdin)) {
			printf("couldn't open /kdev/com1\n");
			exit(1);
		}
		termcook();
		execv("/bin/shell", NULL);
		exit(1);
	}

	if (!fork()) {
		if (!freopen("/vga_tty", "a+", stdout)) {
			printf("couldn't open /vga_tty\n"); // TODO borked
			exit(1);
		}
		if (!freopen("/keyboard", "r", stdin)) {
			printf("couldn't open /keyboard\n");
			exit(1);
		}
		termcook();
		execv("/bin/shell", NULL);
		exit(1);
	}

	_syscall_await();
	printf("init: quitting\n");
	return 0;
}

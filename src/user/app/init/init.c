#include "driver/driver.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <user/lib/fs/misc.h>

void redirect(const char *exe, const char *out, const char *in) {
	if (!fork()) {
		if (!freopen(out, "a+", stdout)) {
			printf("init: couldn't open %s\n", out); // TODO borked
			exit(1);
		}
		if (!freopen(in, "r", stdin)) {
			printf("init: couldn't open %s\n", in);
			exit(1);
		}
		termcook();
		execv(exe, NULL);
		printf("couldn't start %s\n", exe);
		exit(1);
	}
}

int main(void) {
	freopen("/kdev/com1", "a+", stdout);
	printf("in init (stage 2), main at 0x%x\n", &main);

	MOUNT("/tmp/", tmpfs_drv());
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());
	MOUNT("/bin/", fs_passthru("/init/bin"));

	if (fork()) {
		/* used to trigger a kernel bug
		 * 7c96f9c03502e0c60f23f4c550d12a629f3b3daf */
		_syscall_await();
		exit(1);
	}

	redirect("/bin/shell", "/kdev/com1", "/kdev/com1");
	redirect("/bin/shell", "/vga_tty", "/keyboard");

	_syscall_await();
	printf("init: quitting\n");
	return 0;
}

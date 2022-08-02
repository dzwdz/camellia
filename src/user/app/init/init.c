#include "driver/driver.h"
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <user/lib/fs/misc.h>

#define die(fmt, ...) do { fprintf(stderr, "init: " fmt, __VA_ARGS__); exit(1); } while (0)

void redirect(const char *exe, const char *out, const char *in) {
	if (!fork()) {
		if (!freopen(out, "a+", stderr)) {
			fprintf(stdout, "couldn't open %s\n", out);
			exit(1);
		}
		if (!freopen(out, "a+", stdout))
			die("couldn't open %s\n", out);
		if (!freopen(in, "r", stdin))
			die(" couldn't open %s\n", in);
		termcook();
		execv(exe, NULL);
		die("couldn't start %s\n", exe);
	}
}

int main(void) {
	freopen("/kdev/com1", "a+", stdout);
	freopen("/kdev/com1", "a+", stderr);
	printf("in init (stage 2), main at 0x%x\n", &main);

	MOUNT_AT("/tmp/") { tmpfs_drv(); }
	MOUNT_AT("/keyboard") { ps2_drv(); }
	MOUNT_AT("/bin/") { fs_passthru("/init/bin"); }
	MOUNT_AT("/vtty") {
		const char *argv[] = {"/bin/vterm", NULL};
		execv(argv[0], (void*)argv);
	}

	if (fork()) {
		/* used to trigger a kernel bug
		 * 7c96f9c03502e0c60f23f4c550d12a629f3b3daf */
		_syscall_await();
		exit(1);
	}

	redirect("/bin/shell", "/kdev/com1", "/kdev/com1");
	redirect("/bin/shell", "/vtty", "/keyboard");

	_syscall_await();
	printf("init: quitting\n");
	return 0;
}

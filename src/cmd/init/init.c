#include "driver/driver.h"
#include <camellia/compat.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <camellia/fs/misc.h>

#define die(fmt, ...) do { fprintf(stderr, "init: " fmt, __VA_ARGS__); exit(1); } while (0)

static char title[128];

void redirect(const char *exe, const char *out, const char *in) {
	if (!fork()) {
		snprintf(title, sizeof title, "sh >%s", out);
		setproctitle(title);

		if (!freopen(out, "a+", stderr)) {
			fprintf(stdout, "couldn't open %s\n", out);
			exit(1);
		}
		if (!freopen(out, "a+", stdout))
			die("couldn't open %s\n", out);
		if (!freopen(in, "r", stdin))
			die(" couldn't open %s\n", in);

		for (;;) {
			if (!fork()) {
				const char *argv[] = {exe, NULL};
				termcook();
				execv(exe, (void*)argv);
				fprintf(stderr, "init: couldn't start %s\n", exe);
				exit(1);
			}
			_sys_await();
			_sys_intr();
			_sys_sleep(1000);
		}
	}
}

int main(void) {
	hid_t killswitch_pipe[2];

	freopen("/kdev/com1", "a+", stdout);
	freopen("/kdev/com1", "a+", stderr);

	MOUNT_AT("/") {
		fs_dirinject2((const char*[]){
			"/keyboard/",
			"/usr/",
			"/bin/",
			"/Users/",
			"/tmp/",
			"/vtty",
			"/net/",
			"/initctl",
			NULL
		});
	}

	MOUNT_AT("/keyboard") {
		MOUNT_AT("/") { fs_whitelist((const char*[]){"/kdev/ps2/kb", NULL}); }
		ps2_drv();
	}
	MOUNT_AT("/usr/") {
		fs_union((const char*[]){
			"/init/usr/",
			NULL
		});
	}
	MOUNT_AT("/bin/") {
		fs_union((const char*[]){
			"/init/bin/amd64/",
			"/init/bin/sh/",
			"/init/usr/bin/",
			"/init/usr/local/bin/",
			NULL
		});
	}
	MOUNT_AT("/Users/") {
		MOUNT_AT("/tmp/") {
			const char *argv[] = {"/bin/tmpfs", NULL};
			execv(argv[0], (void*)argv);
		}
		// TODO a simple union isn't enough here
		fs_union((const char*[]){
			"/tmp/",
			"/init/Users/",
			NULL
		});
	}
	MOUNT_AT("/tmp/") {
		const char *allow[] = {"/bin/tmpfs", NULL};
		const char *argv[] = {"/bin/tmpfs", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}
	MOUNT_AT("/vtty") {
		const char *allow[] = {"/bin/vterm", "/kdev/video/", "/keyboard", "/init/usr/share/fonts/", NULL};
		const char *argv[] = {"/bin/vterm", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}
	MOUNT_AT("/net/") {
		const char *allow[] = {"/bin/netstack", "/kdev/eth", NULL};
		const char *argv[] = {"/bin/netstack", "/kdev/eth", "192.168.0.11", "192.168.0.2", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}

	if (_sys_pipe(killswitch_pipe, 0) < 0) {
		printf("couldn't create the killswitch pipe, quitting...\n");
		return 1;
	}
	MOUNT_AT("/initctl") {
		close(killswitch_pipe[0]);
		initctl_drv(killswitch_pipe[1]);
	}
	close(killswitch_pipe[1]);

	if (!fork()) {
		// TODO close on exec
		close(killswitch_pipe[0]);
		redirect("/bin/shell", "/kdev/com1", "/kdev/com1");
		redirect("/bin/shell", "/vtty", "/keyboard");
		exit(1);
	}

	char buf[128];
	for (;;) {
		if (_sys_read(killswitch_pipe[0], buf, 128, 0) != 4) {
			break;
		}
		if (memcmp(buf, "intr", 4) == 0) {
			_sys_intr();
		} else if (memcmp(buf, "halt", 4) == 0) {
			break;
		}
	}
	printf("[init] intr\n");
	_sys_intr();
	_sys_sleep(1000);
	printf("[init] filicide\n");
	_sys_filicide();
	printf("[init] goodbye\n");
	return 0;
}

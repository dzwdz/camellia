#include "driver/driver.h"
#include <camellia/compat.h>
#include <camellia/flags.h>
#include <camellia/fs/misc.h>
#include <camellia/intr.h>
#include <camellia/syscalls.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void redirect(const char *exe, const char *out, const char *in) {
	if (!fork()) {
		static char title[128];
		snprintf(title, sizeof title, "sh >%s", out);
		setproctitle(title);

		if (!freopen(out, "a+", stderr)) {
			fprintf(stdout, "couldn't open %s\n", out);
			exit(1);
		}
		if (!freopen(out, "a+", stdout))
			err(1, "couldn't open %s", out);
		if (!freopen(in, "r", stdin))
			err(1, "couldn't open %s", in);

		for (;;) {
			if (!fork()) {
				const char *argv[] = {exe, NULL};
				termcook();
				execv(exe, (void*)argv);
				fprintf(stderr, "init: couldn't start %s\n", exe);
				exit(1);
			}
			_sys_await();
			_sys_intr(NULL, 0);
			_sys_sleep(1000);
		}
	}
}

void shutdown(void) {
	printf("[init] intr\n");
	_sys_intr(NULL, 0);
	_sys_sleep(1000);
	printf("[init] filicide\n");
	_sys_filicide();
	printf("[init] goodbye\n");
	exit(0);
}

int main(void) {
	freopen("/dev/com1", "a+", stdout);
	freopen("/dev/com1", "a+", stderr);

	MOUNT_AT("/") {
		fs_dirinject2((const char*[]){
			"/usr/",
			"/bin/",
			"/tmp/",
			"/net/",
			NULL
		});
	}

	MOUNT_AT("/dev/keyboard") {
		MOUNT_AT("/") { fs_whitelist((const char*[]){"/dev/ps2/kb", NULL}); }
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
	MOUNT_AT("/tmp/") {
		const char *allow[] = {"/bin/tmpfs", NULL};
		const char *argv[] = {"/bin/tmpfs", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}
	MOUNT_AT("/dev/vtty") {
		const char *allow[] = {"/bin/vterm", "/dev/video/", "/dev/keyboard", "/init/usr/share/fonts/", NULL};
		const char *argv[] = {"/bin/vterm", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}
	MOUNT_AT("/net/") {
		const char *allow[] = {"/bin/netstack", "/dev/eth", NULL};
		const char *argv[] = {"/bin/netstack", "/dev/eth", "192.168.0.11", "192.168.0.2", NULL};
		MOUNT_AT("/") { fs_whitelist(allow); }
		execv(argv[0], (void*)argv);
	}

	if (!fork()) {
		redirect("/bin/shell", "/dev/com1", "/dev/com1");
		redirect("/bin/shell", "/dev/vtty", "/dev/keyboard");
		exit(1);
	}

	intr_set(shutdown);
	for (;;) {
		// TODO sleep(-1) to sleep forever
		// TODO maybe init should collect dead children?
		_sys_sleep(86400000);
	}
}

#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool fork2_n_mount(const char *path);

void fs_passthru(const char *prefix);
void fs_whitelist(const char **list);

void fs_dir_inject(const char *path);

static bool mount_at_pred(const char *path) {
	// TODO preprocess path - simplify & ensure trailing slash
	if (!fork2_n_mount(path)) {
		/* child -> go into the for body */
		_klogf("%s: impl", path);
		return true;
	}

	if (strcmp("/", path) && !fork2_n_mount("/")) {
		_klogf("%s: dir", path);
		fs_dir_inject(path);
		exit(1);
	}
	return false; /* continue after the for loop */
}

/** Mounts something and injects its path into the fs */
#define MOUNT_AT(path) for (; mount_at_pred(path); exit(1))

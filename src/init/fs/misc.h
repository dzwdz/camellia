#pragma once
#include <stdbool.h>

bool fork2_n_mount(const char *path);

void fs_passthru(const char *prefix);

void fs_dir_inject(const char *path);

/** Mounts something and injects its path into the fs */
// TODO path needs to have a trailing slash
#define MOUNT(path, impl) \
	if (!fork2_n_mount(path)) {impl;} \
	if (!fork2_n_mount("/"))  fs_dir_inject(path);

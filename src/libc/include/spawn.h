#pragma once
#include <sys/types.h>

typedef struct {
	struct file_action *f;
} posix_spawn_file_actions_t;

typedef struct {} posix_spawnattr_t;

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *facts, int from, int to);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *facts);
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *facts);

int posix_spawnp(
	pid_t *restrict pid,
	const char *restrict file,
	const posix_spawn_file_actions_t *restrict file_actions,
	const posix_spawnattr_t *restrict attrp,
	char *const argv[restrict],
	char *const envp[restrict]
);

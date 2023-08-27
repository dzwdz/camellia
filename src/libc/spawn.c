#include <camellia/syscalls.h>
#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <unistd.h>

enum act {
	ACT_DUP2 = 1,
};

struct file_action {
	struct file_action *next;
	enum act type;
	int a, b;
};

int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *facts, int from, int to)
{
	struct file_action *fact, **tail;
	fact = calloc(1, sizeof(*fact));
	if (!fact) return ENOMEM;

	fact->type = ACT_DUP2;
	fact->a = from;
	fact->b = to;

	tail = &facts->f;
	while (*tail) {
		tail = &(*tail)->next;
	}
	*tail = fact;
	return 0;
}

int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *facts)
{
	while (facts->f) {
		struct file_action *cur = facts->f;
		facts->f = cur->next;
		free(cur);
	}
	return 0;
}

int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *facts)
{
	facts->f = NULL;
	return 0;
}

int
posix_spawnp(
	pid_t *restrict pidp,
	const char *restrict file,
	const posix_spawn_file_actions_t *restrict facts,
	const posix_spawnattr_t *restrict attrp,
	char *const argv[restrict],
	char *const envp[restrict]
) {
	if (attrp) return ENOSYS;

	pid_t pid = fork();
	if (pid < 0) return errno;
	if (pid == 0) {
		struct file_action *fact = facts ? facts->f : NULL;
		while (fact) {
			switch (fact->type) {
			case ACT_DUP2:
				dup2(fact->a, fact->b);
				break;
			}
			fact = fact->next;
		}
		execvpe(file, argv, envp);
		_sys_exit(127);
	}
	if (pidp) *pidp = pid;
	return 0;
}

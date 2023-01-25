#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <unistd.h>
#include <user/lib/esemaphore.h>

void esem_signal(struct evil_sem *sem) {
	_sys_write(sem->signal, NULL, 0, 0, 0);
}

void esem_wait(struct evil_sem *sem) {
	_sys_read(sem->wait, NULL, 0, 0);
}

struct evil_sem *esem_new(int value) {
	hid_t ends_wait[2], ends_signal[2];
	struct evil_sem *sem;

	if (value < 0) return NULL;
	if (_sys_pipe(ends_wait, 0) < 0) return NULL;
	if (_sys_pipe(ends_signal, 0) < 0) goto fail_signal;
	if (!(sem = malloc(sizeof *sem))) goto fail_malloc;

	if (!_sys_fork(FORK_NOREAP, NULL)) {
		close(ends_signal[1]);
		while (_sys_read(ends_signal[0], NULL, 0, 0) >= 0) {
			if (!_sys_fork(FORK_NOREAP, NULL)) {
				_sys_write(ends_wait[1], NULL, 0, 0, 0);
				exit(0);
			}
		}
		exit(0);
	}
	close(ends_signal[0]);
	close(ends_wait[1]);

	sem->wait = ends_wait[0];
	sem->signal = ends_signal[1];

	while (value--) esem_signal(sem);
	return sem;

fail_malloc:
	close(ends_signal[0]);
	close(ends_signal[1]);
fail_signal:
	close(ends_wait[0]);
	close(ends_wait[1]);
	return NULL;
}

void esem_free(struct evil_sem *sem) {
	close(sem->wait);
	close(sem->signal);
	free(sem);
}

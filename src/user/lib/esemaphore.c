#include <shared/flags.h>
#include <shared/syscalls.h>
#include <user/lib/esemaphore.h>
#include <user/lib/stdlib.h>

void esem_signal(struct evil_sem *sem) {
	_syscall_write(sem->signal, NULL, 0, 0);
}

void esem_wait(struct evil_sem *sem) {
	_syscall_read(sem->wait, NULL, 0, 0);
}

struct evil_sem *esem_new(int value) {
	handle_t ends_wait[2], ends_signal[2];
	struct evil_sem *sem;

	if (value < 0) return NULL;
	if (_syscall_pipe(ends_wait, 0) < 0) return NULL;
	if (_syscall_pipe(ends_signal, 0) < 0) goto fail_signal;
	if (!(sem = malloc(sizeof *sem))) goto fail_malloc;

	if (!_syscall_fork(FORK_NOREAP, NULL)) {
		close(ends_signal[1]);
		while (_syscall_read(ends_signal[0], NULL, 0, 0) >= 0) {
			if (!_syscall_fork(FORK_NOREAP, NULL)) {
				_syscall_write(ends_wait[1], NULL, 0, 0);
				_syscall_exit(0);
			}
		}
		_syscall_exit(0);
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

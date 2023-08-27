#include <bits/panic.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>

pid_t wait(int *wstatus) {
	return wait3(wstatus, 0, NULL);
}

pid_t wait3(int *wstatus, int opts, struct rusage *rusage) {
	if (rusage) {
		__libc_panic("unimplemented");
	}
	return waitpid(-1, wstatus, opts);
}

pid_t waitpid(pid_t pid, int *wstatus, int opts) {
	struct sys_wait2 res;
	if (opts) {
		__libc_panic("unimplemented");
	}
	pid_t ret = _sys_wait2(pid, 0, &res);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	if (wstatus) {
		*wstatus = res.status & 0xFF;
	}
	return ret;
}

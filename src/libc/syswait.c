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
	struct sys_wait2 res;
	if (opts || rusage) {
		__libc_panic("unimplemented");
	}
	pid_t ret = _sys_wait2(-1, 0, &res);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	if (wstatus) {
		*wstatus = res.status & 0xFF;
	}
	return ret;
}

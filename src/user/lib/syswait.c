#include <bits/panic.h>
#include <sys/resource.h>
#include <sys/wait.h>

pid_t wait3(int *wstatus, int opts, struct rusage *rusage) {
	(void)wstatus; (void)opts; (void)rusage;
	__libc_panic("unimplemented");
}

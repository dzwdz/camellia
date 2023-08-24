#include <errno.h>
#include <time.h>
#include <utime.h>

// TODO time
time_t time(time_t *tloc) {
	time_t ret = 0;
	if (tloc) *tloc = ret;
	return ret;
}

clock_t clock(void) {
	return 0;
}

struct tm *gmtime(const time_t *timer) {
	(void)timer;
	errno = ENOSYS;
	return NULL;
}

struct tm *localtime(const time_t *timer) {
	(void)timer;
	errno = ENOSYS;
	return NULL;
}

time_t mktime(struct tm *timeptr) {
	(void)timeptr;
	return 0;
}

double difftime(time_t time1, time_t time0) {
	(void)time1; (void)time0;
	return 0;
}

char *ctime(const time_t *timep) {
	(void)timep;
	return "THE FUTURE";
}

size_t strftime(
	char *restrict s, size_t maxsize,
	const char *restrict format, const struct tm *restrict timeptr)
{
	(void)s; (void)maxsize; (void)format; (void)timeptr;
	return 0;
}

int utime(const char *fn, const struct utimbuf *times) {
	(void)fn; (void)times;
	errno = ENOSYS;
	return -1;
}

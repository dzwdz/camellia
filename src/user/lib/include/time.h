#pragma once
#include <sys/types.h>

#define CLOCKS_PER_SEC 1000000

struct tm {
	int tm_sec;    /* Seconds [0,60]. */
	int tm_min;    /* Minutes [0,59]. */
	int tm_hour;   /* Hour [0,23]. */
	int tm_mday;   /* Day of month [1,31]. */
	int tm_mon;    /* Month of year [0,11]. */
	int tm_year;   /* Years since 1900. */
	int tm_wday;   /* Day of week [0,6] (Sunday =0). */
	int tm_yday;   /* Day of year [0,365]. */
	int tm_isdst;  /* Daylight Savings flag. */
};

time_t time(time_t *tloc);
clock_t clock(void);

struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t mktime(struct tm *timeptr);

double difftime(time_t time1, time_t time0);

size_t strftime(
	char *restrict s, size_t maxsize,
	const char *restrict format, const struct tm *restrict timeptr);

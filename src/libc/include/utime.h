#pragma once
#include <sys/time.h>

struct utimbuf {
	time_t actime;
	time_t modtime;
};

int utime(const char *fn, const struct utimbuf *times);

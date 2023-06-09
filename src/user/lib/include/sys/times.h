#pragma once
#include <bits/panic.h>

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

static inline clock_t times(struct tms *buf) {
	__libc_panic("unimplemented");
}

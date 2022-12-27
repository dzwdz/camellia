#pragma once
#include <stdarg.h>

_Noreturn void err(int ret, const char *fmt, ...);
_Noreturn void errx(int ret, const char *fmt, ...);
void warn(const char *fmt, ...);
void warnx(const char *fmt, ...);

void vwarn(const char *fmt, va_list args);
void vwarnx(const char *fmt, va_list args);

#pragma once
#include <stdarg.h>
#include <stddef.h>

int __printf_internal(const char *fmt, va_list argp,
		void (*back)(void*, const char*, size_t), void *back_arg);

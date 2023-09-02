#include <bits/panic.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

_Noreturn void abort(void) {
	_sys_exit(1);
}

char *mktemp(char *tmpl) {
	// TODO mktemp mkstemp
	return tmpl;
}

int mkstemp(char *tmpl) {
	hid_t h = camellia_open(tmpl, OPEN_CREATE | OPEN_RW);
	if (h < 0) {
		errno = -h;
		return -1;
	}
	// TODO truncate
	return h;
}

// TODO process env
char *getenv(const char *name) {
	(void)name;
	return NULL;
}

// TODO system()
int system(const char *cmd) {
	(void)cmd;
	errno = ENOSYS;
	return -1;
}

int abs(int i) {
	return i < 0 ? -i : i;
}

int atoi(const char *s) {
	return strtol(s, NULL, 10);
}

long atol(const char *s) {
	return strtol(s, NULL, 10);
}

double atof(const char *s) {
	(void)s;
	__libc_panic("unimplemented");
}

static unsigned long long
strton(const char *restrict s, char **restrict end, int base, int *sign)
{
	long res = 0;

	while (isspace(*s)) s++;

	if (sign) *sign = 1;
	if (*s == '+') {
		s++;
	} else if (*s == '-') {
		s++;
		if (sign) *sign = -1;
	}

	if (base == 0) {
		if (*s == '0') {
			s++;
			if (*s == 'x' || *s == 'X') {
				s++;
				base = 16;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	}

	for (;;) {
		unsigned char digit = *s;
		if      ('0' <= digit && digit <= '9') digit -= '0';
		else if ('a' <= digit && digit <= 'z') digit -= 'a' - 10;
		else if ('A' <= digit && digit <= 'Z') digit -= 'A' - 10;
		else break;

		if (digit >= base) break;
		// TODO overflow check
		res *= base;
		res += digit;

		s++;
	}
	if (end) *end = (void*)s;
	return res;
}

long strtol(const char *restrict s, char **restrict end, int base) {
	int sign;
	long n = strton(s, end, base, &sign);
	return n * sign;
}

long long strtoll(const char *restrict s, char **restrict end, int base) {
	int sign;
	long long n = strton(s, end, base, &sign);
	return n * sign;
}

unsigned long strtoul(const char *restrict s, char **restrict end, int base) {
	return strton(s, end, base, NULL);
}

unsigned long long strtoull(const char *restrict s, char **restrict end, int base) {
	return strton(s, end, base, NULL);
}

double strtod(const char *restrict s, char **restrict end) {
	(void)s; (void)end;
	__libc_panic("unimplemented");
}

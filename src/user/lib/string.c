#include <errno.h>
#include <string.h>

int isspace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

long strtol(const char *restrict s, char **restrict end, int base) {
	long res = 0;
	int sign = 1;

	while (isspace(*s)) s++;

	if (*s == '+') {
		s++;
	} else if (*s == '-') {
		s++;
		sign = -1;
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
	return res * sign;
}

char *strchr(const char *s, int c) {
	while (*s) {
		if (*s == c) return (char*)s;
		s++;
	}
	return NULL;
}

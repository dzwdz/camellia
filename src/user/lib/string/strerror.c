#include <string.h>

static const char *errstr[] = {
#	define E(n, str) [n] = str,
#	include <__errno.h>
#	undef E
};

char *strerror(int n) {
	if (0 <= n && n * sizeof(*errstr) < sizeof(errstr) && errstr[n])
		return (char*)errstr[n];
	return "unknown error";
}

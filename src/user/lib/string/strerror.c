#include <string.h>

static const char *errstr[] = {
#	define E(n, str) [n] = str,
#	include <__errno.h>
#	undef E
};

char *strerror(int n) {
	return (char*)(errstr[n] ? errstr[n] : "unknown error");
}

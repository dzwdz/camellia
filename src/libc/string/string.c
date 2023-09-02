/** String functions that don't fit into any of the other .c files */

#include <bits/panic.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

char *strncat(char *restrict dst, const char *restrict src, size_t n) {
	(void)dst; (void)src; (void)n;
	__libc_panic("unimplemented");
}

char *stpncpy(char *restrict dst, const char *restrict src, size_t n) {
	(void)dst; (void)src; (void)n;
	__libc_panic("unimplemented");
}

char *strdup(const char *s) {
	size_t len = strlen(s) + 1;
	char *buf = malloc(len);
	if (buf) memcpy(buf, s, len);
	return buf;
}

char *strsignal(int sig) {
	static char buf[32];
	snprintf(buf, sizeof(buf), "signal %d", sig);
	return buf;
}

/* strings.h */
int strcasecmp(const char *s1, const char *s2) {
	return strncasecmp(s1, s2, ~0);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		char c1 = tolower(s1[i]), c2 = tolower(s2[i]);
		if (c1 == '\0' || c1 != c2) {
			if (c1 < c2)      return -1;
			else if (c1 > c2) return 1;
			else              return 0;
		}
	}
	return 0;
}
